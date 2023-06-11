//
// Created by Tyler Gilbert on 6/11/23.
//

#include "bump_allocator.h"

static bump_allocator_t bump_allocator;

void codec2_set_heap(void * heap, size_t heap_size){
    bump_allocator_initialize(&bump_allocator, heap, heap_size);
}

void codec2_set_heap_changed_callback(void (*handler)(const char *)){
    bump_allocator_set_on_changed_handler(&bump_allocator, handler);
}


size_t codec2_get_heap_count(){
    return bump_allocator_get_count(&bump_allocator);
}

size_t codec2_get_maximum_usage(){
    return bump_allocator_get_maximum_usage(&bump_allocator);
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
