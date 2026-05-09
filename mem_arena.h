// mem_arena.h
#ifndef MEM_ARENA_H
#define MEM_ARENA_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Arena struct
typedef struct Arena Arena;
struct Arena {
    uint8_t* base;
    size_t cap;
    size_t offset;
};

// Arena functions
void arena_init(Arena *a, void *start, size_t cap);
// inline arena funcs
static inline void *arena_alloc(Arena *a, size_t size, size_t align) {
    size_t p =(a->offset + (align - 1)) & ~(align - 1); // alignment math, offset + align-1 bitwise ANDed against bitmask of align-1
    if (p+size > a->cap) return NULL; // out of memory check
    void *ptr = a->base + p; // compute actual pointer
    a->offset = p + size; // advance arena's offset
    return ptr; // return new pointer
}

static inline void arena_reset(Arena *a) {
    a->offset = 0;
}

static inline char* arena_strdup(Arena* a, const char* str) {
    size_t len = strlen(str) + 1;
    char* copy = (char*)arena_alloc(a, len, 1); // alignment 1 for chars
    if (copy) {
        memcpy(copy, str, len);
    }
    return copy;
}

#endif // MEM_ARENA_H