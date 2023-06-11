

#ifndef __BUMP_ALLOCATOR__
#define  __BUMP_ALLOCATOR__


//
// Created by Tyler Gilbert on 6/11/23.
//


#include <stddef.h>
#include <stdint.h>

#define ALIGNMENT 8

typedef struct {
    const char * action;
    size_t count;
    size_t usage;
    size_t maximum_usage;
} bump_allocator_status_t;

typedef struct {
    size_t count;
    size_t head;
    size_t maximum_usage;
    void (*on_changed)(const char *);
    uint8_t * heap;
    size_t heap_size;
} bump_allocator_t;

void bump_allocator_initialize(bump_allocator_t * self, uint8_t * heap, size_t heap_size);
void bump_allocator_set_on_changed_handler(bump_allocator_t* self, void (*handler)(const char *));
size_t bump_allocator_get_count(bump_allocator_t* self);
size_t bump_allocator_get_maximum_usage(bump_allocator_t* self);

void* bump_allocator_malloc(bump_allocator_t* self, size_t size);
void bump_allocator_free(bump_allocator_t* self, void* ptr);



#endif