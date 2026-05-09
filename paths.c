#include "paths.h"

#include <SDL2/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

static bool get_cwd(char *buf, size_t sz) {
#ifdef _WIN32
    return _getcwd(buf, (int)sz) != NULL;
#else
    return getcwd(buf, sz) != NULL;
#endif
}

#ifndef PATH_MAX_CANDIDATES
#define PATH_MAX_CANDIDATES 8
#endif

static char s_roots[PATH_MAX_CANDIDATES][1024];
static int s_root_count;

static bool is_path_absolute(const char *p) {
    if (!p || !p[0]) {
        return false;
    }
#ifdef _WIN32
    if (p[0] == '/' || p[0] == '\\') {
        return true;
    }
    if (((unsigned char)p[0] >= 'A' && (unsigned char)p[0] <= 'Z') ||
        ((unsigned char)p[0] >= 'a' && (unsigned char)p[0] <= 'z')) {
        return p[1] == ':' && (p[2] == '/' || p[2] == '\\');
    }
    return false;
#else
    return p[0] == '/';
#endif
}

static void strip_trailing_seps(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '/' || s[n - 1] == '\\')) {
        s[--n] = '\0';
    }
}

static void ensure_trailing_sep(char *s, size_t cap) {
    size_t n = strlen(s);
    if (n == 0 || n + 2 > cap) {
        return;
    }
    if (s[n - 1] != '/' && s[n - 1] != '\\') {
        s[n] = '/';
        s[n + 1] = '\0';
    }
}

static bool path_parent_dir(char *path) {
    strip_trailing_seps(path);
    size_t n = strlen(path);
    if (n == 0) {
        return false;
    }
    char *last = strrchr(path, '/');
    if (!last) {
        last = strrchr(path, '\\');
    }
    if (!last || last == path) {
        return false;
    }
    *last = '\0';
    ensure_trailing_sep(path, sizeof(s_roots[0]));
    return true;
}

static bool file_readable(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        return false;
    }
    fclose(f);
    return true;
}

static void try_add_root(const char *root) {
    if (!root || s_root_count >= PATH_MAX_CANDIDATES) {
        return;
    }
    char tmp[1024];
    snprintf(tmp, sizeof(tmp), "%s", root);
    if (tmp[0] == '\0') {
        return;
    }
    ensure_trailing_sep(tmp, sizeof(tmp));

    for (int i = 0; i < s_root_count; i++) {
        if (strcmp(s_roots[i], tmp) == 0) {
            return;
        }
    }
    snprintf(s_roots[s_root_count], sizeof(s_roots[s_root_count]), "%s", tmp);
    s_root_count++;
}

bool paths_init(void) {
    s_root_count = 0;

    const char *env = getenv("PSEUDO_RACER_RESOURCE_ROOT");
    if (env && env[0]) {
        try_add_root(env);
    }

    char *base = SDL_GetBasePath();
    if (base) {
        try_add_root(base);
        char parent[1024];
        snprintf(parent, sizeof(parent), "%s", base);
        SDL_free(base);
        if (path_parent_dir(parent)) {
            try_add_root(parent);
        }
    } else {
        try_add_root(".");
    }

    char cwd[1024];
    if (get_cwd(cwd, sizeof(cwd))) {
        try_add_root(cwd);
    }

    if (s_root_count == 0) {
        try_add_root(".");
    }

    return s_root_count > 0;
}

void paths_quit(void) {
    s_root_count = 0;
}

static void path_join_to(char *out, size_t out_sz, const char *root, const char *rel) {
    while (rel[0] == '.' && rel[1] == '/') {
        rel += 2;
    }
    snprintf(out, out_sz, "%s%s", root, rel);
}

bool paths_resolve(char *out, size_t out_sz, const char *rel_path) {
    if (!out || out_sz == 0 || !rel_path) {
        return false;
    }

    if (is_path_absolute(rel_path)) {
        snprintf(out, out_sz, "%s", rel_path);
        return file_readable(out);
    }

    for (int i = 0; i < s_root_count; i++) {
        path_join_to(out, out_sz, s_roots[i], rel_path);
        if (file_readable(out)) {
            return true;
        }
    }

    if (s_root_count > 0) {
        path_join_to(out, out_sz, s_roots[0], rel_path);
    } else {
        snprintf(out, out_sz, "%s", rel_path);
    }
    return false;
}
