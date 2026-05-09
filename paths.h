#ifndef PATHS_H
#define PATHS_H

#include <stddef.h>
#include <stdbool.h>

/* Call once after SDL_Init (SDL_GetBasePath is safe earlier, but we keep init with the app). */
bool paths_init(void);
void paths_quit(void);

/*
 * Write a path to `out` for opening `rel_path` (relative to resource root).
 * Tries, in order: PSEUDO_RACER_RESOURCE_ROOT (if set), SDL_GetBasePath(),
 * parent of base path, then current working directory.
 * If `rel_path` is absolute, it is copied to `out` and existence is checked only.
 * Returns true if the file at `out` exists and is readable.
 */
bool paths_resolve(char *out, size_t out_sz, const char *rel_path);

#endif
