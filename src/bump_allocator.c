//
// Created by Tyler Gilbert on 6/11/23.
//


#include <stddef.h>

#include "bump_allocator.h"

void bump_allocator_initialize(bump_allocator_t * self, uint8_t * heap, size_t heap_size){
    *self = (bump_allocator_t){0};
    self->heap = heap;
    self->heap_size = heap_size;
}

void bump_allocator_set_on_changed_handler(bump_allocator_t* self, void (*handler)(const char*)) {
    self->on_changed = handler;
}

size_t bump_allocator_get_count(bump_allocator_t* self) {
    return self->count;
}

size_t bump_allocator_get_maximum_usage(bump_allocator_t* self) {
    return self->maximum_usage;
}

void bump_allocator_changed(bump_allocator_t* self, const char * action) {
    if (self->on_changed) {
        self->on_changed(action);
    }
}

void* bump_allocator_malloc(bump_allocator_t* self, size_t size) {
    size_t next_head = self->head + ((size + ALIGNMENT - 1) / ALIGNMENT) * ALIGNMENT;
    if (next_head > self->heap_size) {
        bump_allocator_changed(self, "error");
        return NULL;
    }
    void* result = &self->heap[self->head];
    self->head = next_head;
    if (self->maximum_usage < self->head) {
        self->maximum_usage = self->head;
    }
    self->count++;
    bump_allocator_changed(self, "malloc");
    return result;
}

void bump_allocator_free(bump_allocator_t* self, void* ptr) {
    (void)ptr;
    if (self->count > 0) {
        self->count--;
        if (self->count == 0) {
            self->head = 0;
        }
    }
    bump_allocator_changed(self, "free");
}

