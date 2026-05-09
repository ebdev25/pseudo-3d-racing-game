// mem_arena.c
#include "mem_arena.h"

void arena_init(Arena *a, void *start, size_t cap) {
    a->base = (uint8_t *)start;
    a->cap = cap;
    a->offset = 0;
}