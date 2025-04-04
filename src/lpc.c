/*---------------------------------------------------------------------------*\

  FILE........: lpc.c
  AUTHOR......: David Rowe
  DATE CREATED: 30 Sep 1990 (!)

  Linear Prediction functions written in C.

\*---------------------------------------------------------------------------*/

/*
  Copyright (C) 2009-2012 David Rowe

  All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 2.1, as
  published by the Free Software Foundation.  This program is
  distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
  License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this program; if not, see <http://www.gnu.org/licenses/>.
*/

#define LPC_MAX_N 512		/* maximum no. of samples in frame */
#define PI 3.141592654		/* mathematical constant */

#define ALPHA 1.0
#define BETA  0.94

#include <assert.h>
#include <math.h>
#include "defines.h"
#include "lpc.h"

/*---------------------------------------------------------------------------*\

  pre_emp()

  Pre-emphasise (high pass filter with zero close to 0 Hz) a frame of
  speech samples.  Helps reduce dynamic range of LPC spectrum, giving
  greater weight and hense a better match to low energy formants.

  Should be balanced by de-emphasis of the output speech.

\*---------------------------------------------------------------------------*/

void pre_emp(
  float  Sn_pre[], /* output frame of speech samples                     */
  float  Sn[],	   /* input frame of speech samples                      */
  float *mem,      /* Sn[-1]single sample memory                         */
  int   Nsam	   /* number of speech samples to use                    */
)
{
    int   i;

    for(i=0; i<Nsam; i++) {
	Sn_pre[i] = Sn[i] - ALPHA * mem[0];
	mem[0] = Sn[i];
    }

}


/*---------------------------------------------------------------------------*\

  de_emp()

  De-emphasis filter (low pass filter with a pole close to 0 Hz).

\*---------------------------------------------------------------------------*/

void de_emp(
  float  Sn_de[],  /* output frame of speech samples                     */
  float  Sn[],	   /* input frame of speech samples                      */
  float *mem,      /* Sn[-1]single sample memory                         */
  int    Nsam	   /* number of speech samples to use                    */
)
{
    int   i;

    for(i=0; i<Nsam; i++) {
	Sn_de[i] = Sn[i] + BETA * mem[0];
	mem[0] = Sn_de[i];
    }

}


/*---------------------------------------------------------------------------*\

  hanning_window()

  Hanning windows a frame of speech samples.

\*---------------------------------------------------------------------------*/

void hanning_window(
  float Sn[],	/* input frame of speech samples */
  float Wn[],	/* output frame of windowed samples */
  int Nsam	/* number of samples */
)
{
  int i;	/* loop variable */

  for(i=0; i<Nsam; i++)
    Wn[i] = Sn[i]*(0.5f - 0.5f*cosf(2*PI*(float)i/(Nsam-1)));
}

/*---------------------------------------------------------------------------*\

  autocorrelate()

  Finds the first P autocorrelation values of an array of windowed speech
  samples Sn[].

\*---------------------------------------------------------------------------*/

void autocorrelate(
  float Sn[],	/* frame of Nsam windowed speech samples */
  float Rn[],	/* array of P+1 autocorrelation coefficients */
  int Nsam,	/* number of windowed samples to use */
  int order	/* order of LPC analysis */
)
{
  int i,j;	/* loop variables */

  for(j=0; j<order+1; j++) {
    Rn[j] = 0.0f;
    for(i=0; i<Nsam-j; i++)
      Rn[j] += Sn[i]*Sn[i+j];
  }
}

/*---------------------------------------------------------------------------*\

  levinson_durbin()

  Given P+1 autocorrelation coefficients, finds P Linear Prediction Coeff.
  (LPCs) where P is the order of the LPC all-pole model. The Levinson-Durbin
  algorithm is used, and is described in:

    J. Makhoul
    "Linear prediction, a tutorial review"
    Proceedings of the IEEE
    Vol-63, No. 4, April 1975

\*---------------------------------------------------------------------------*/

void levinson_durbin(
  float R[],		/* order+1 autocorrelation coeff */
  float lpcs[],		/* order+1 LPC's */
  int order		/* order of the LPC analysis */
)
{
  float a[order+1][order+1];
  float sum, e, k;
  int i,j;				/* loop variables */

  e = R[0];				/* Equation 38a, Makhoul */

  for(i=1; i<=order; i++) {
    sum = 0.0f;
    for(j=1; j<=i-1; j++)
      sum += a[i-1][j]*R[i-j];
    k = -1.0f*(R[i] + sum)/e;		/* Equation 38b, Makhoul */
    if (fabsf(k) > 1.0f)
      k = 0.0f;

    a[i][i] = k;

    for(j=1; j<=i-1; j++)
      a[i][j] = a[i-1][j] + k*a[i-1][i-j];	/* Equation 38c, Makhoul */

    e *= (1-k*k);				/* Equation 38d, Makhoul */
  }

  for(i=1; i<=order; i++)
    lpcs[i] = a[order][i];
  lpcs[0] = 1.0f;
}

/*---------------------------------------------------------------------------*\

  inverse_filter()

  Inverse Filter, A(z).  Produces an array of residual samples from an array
  of input samples and linear prediction coefficients.

  The filter memory is stored in the first order samples of the input array.

\*---------------------------------------------------------------------------*/

void inverse_filter(
  float Sn[],	/* Nsam input samples */
  float a[],	/* LPCs for this frame of samples */
  int Nsam,	/* number of samples */
  float res[],	/* Nsam residual samples */
  int order	/* order of LPC */
)
{
  int i,j;	/* loop variables */

  for(i=0; i<Nsam; i++) {
    res[i] = 0.0f;
    for(j=0; j<=order; j++)
      res[i] += Sn[i-j]*a[j];
  }
}

/*---------------------------------------------------------------------------*\

 synthesis_filter()

 C version of the Speech Synthesis Filter, 1/A(z).  Given an array of
 residual or excitation samples, and the the LP filter coefficients, this
 function will produce an array of speech samples.  This filter structure is
 IIR.

 The synthesis filter has memory as well, this is treated in the same way
 as the memory for the inverse filter (see inverse_filter() notes above).
 The difference is that the memory for the synthesis filter is stored in
 the output array, whereas the memory of the inverse filter is stored in the
 input array.

 Note: the calling function must update the filter memory.

\*---------------------------------------------------------------------------*/

void synthesis_filter(
  float res[],	/* Nsam input residual (excitation) samples */
  float a[],	/* LPCs for this frame of speech samples */
  int Nsam,	/* number of speech samples */
  int order,	/* LPC order */
  float Sn_[]	/* Nsam output synthesised speech samples */
)
{
  int i,j;	/* loop variables */

  /* Filter Nsam samples */

  for(i=0; i<Nsam; i++) {
    Sn_[i] = res[i]*a[0];
    for(j=1; j<=order; j++)
      Sn_[i] -= Sn_[i-j]*a[j];
  }
}

/*---------------------------------------------------------------------------*\

  find_aks()

  This function takes a frame of samples, and determines the linear
  prediction coefficients for that frame of samples.

\*---------------------------------------------------------------------------*/

void find_aks(
  float Sn[],	/* Nsam samples with order sample memory */
  float a[],	/* order+1 LPCs with first coeff 1.0 */
  int Nsam,	/* number of input speech samples */
  int order,	/* order of the LPC analysis */
  float *E	/* residual energy */
)
{
  float Wn[LPC_MAX_N];	/* windowed frame of Nsam speech samples */
  float R[order+1];	/* order+1 autocorrelation values of Sn[] */
  int i;

  assert(Nsam < LPC_MAX_N);

  hanning_window(Sn,Wn,Nsam);
  autocorrelate(Wn,R,Nsam,order);
  levinson_durbin(R,a,order);

  *E = 0.0f;
  for(i=0; i<=order; i++)
    *E += a[i]*R[i];
  if (*E < 0.0f)
    *E = 1E-12f;
}

/*---------------------------------------------------------------------------*\

  weight()

  Weights a vector of LPCs.

\*---------------------------------------------------------------------------*/

void weight(
  float ak[],	/* vector of order+1 LPCs */
  float gamma,	/* weighting factor */
  int order,	/* num LPCs (excluding leading 1.0) */
  float akw[]	/* weighted vector of order+1 LPCs */
)
{
  int i;

  for(i=1; i<=order; i++)
    akw[i] = ak[i]*powf(gamma,(float)i);
}

