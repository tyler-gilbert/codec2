//
// Created by Tyler Gilbert on 6/11/23.
//

#include "bump_allocator.h"
#include "codec2.h"

static bump_allocator_t bump_allocator;

void codec2_set_heap(void * heap, unsigned int heap_size){
    if(bump_allocator.heap == NULL) {
        bump_allocator_initialize(&bump_allocator, heap, (size_t) heap_size);
    }
}

void codec2_set_heap_changed_callback(void (*handler)(const char *)){
    bump_allocator_set_on_changed_handler(&bump_allocator, handler);
}

void codec2_get_heap_info(heap_info_t * dest){
    if( bump_allocator.heap == NULL ){
        *dest = (heap_info_t){0};
        return;
    }

    dest->count = bump_allocator.count;
    dest->head = bump_allocator.head;
    dest->maximum_usage = bump_allocator.maximum_usage;
    dest->size = bump_allocator.heap_size;
}

void* codec2_malloc(size_t size){
    return bump_allocator_malloc(&bump_allocator, size);
}

void* codec2_calloc(size_t nmemb, size_t size){
    return bump_allocator_malloc(&bump_allocator, nmemb*size);
}

void codec2_free(void* ptr){
    bump_allocator_free(&bump_allocator, ptr);
}
