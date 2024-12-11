// util.c
#include "util.h"
#include <math.h>

// Returns the current time in seconds since the program started
double timestamp() {
    return SDL_GetTicks() / 1000.0;
}

// Constrains a value within a specified range [min, max]
double limit(double value, double min, double max) {
    return fmax(min, fmin(value, max));
}

// Calculates the new velocity after applying acceleration over time dt
double accelerate(double v, double accel, double dt) {
    return v + (accel * dt);
}

// Linearly interpolates between two values (a and b) by a percentage (0-1)
double interpolate(double a, double b, double percent) {
    return a + (b - a) * percent;
}

// Performs quadratic easing in for smoother start of interpolation from a to b
double easeIn(double a, double b, double percent) {
    return a + (b - a) * pow(percent, 2);
}

// Performs quadratic easing out for smoother end of interpolation from a to b
double easeOut(double a, double b, double percent) {
    return a + (b - a) * (1 - pow(1 - percent, 2));
}

// Performs quadratic easing in and out for smoother start and end of interpolation
double easeInOut(double a, double b, double percent) {
    return a + (b - a) * ((-cos(percent * M_PI) / 2) + 0.5);
}

// Computes fog intensity based on distance and density for exponential fog effect
double exponentialFog(double distance, double density) {
    return 1.0 / (exp(pow(distance, 2) * density));
}

// Increments a value by a specified amount, wrapping around max if it exceeds max
double increase(double start, double increment, double max) {
    double result = start + increment;
    while (result >= max)
        result -= max;
    while (result < 0)
        result += max;
    return result;
}

// Projects a 3D world point onto 2D screen space based on camera position and perspective settings
void project(Point* p, double cameraX, double cameraY, double cameraZ, double cameraDepth, int width, int height, double roadWidth) {
    
    // Translate the point's world coordinates to camera coordinates by subtracting the camera's position
    p->camera.x = p->world.x - cameraX;  // X-axis translation relative to the camera
    p->camera.y = p->world.y - cameraY;  // Y-axis translation relative to the camera
    p->camera.z = p->world.z - cameraZ;  // Z-axis translation (depth) relative to the camera

    // Calculate the scale factor based on distance from the camera (Z-axis depth).
    // As depth (camera.z) increases, scale decreases, creating perspective effect.
    p->scale = cameraDepth / p->camera.z;

    // Project the X coordinate onto screen space.
    // Center the X position in the middle of the screen, then apply scaling based on distance.
    p->screen.x = round((width / 2.0) + (p->scale * p->camera.x * width / 2.0));

    // Project the Y coordinate onto screen space.
    // Center the Y position vertically on the screen, then apply scaling based on distance.
    p->screen.y = round((height / 2.0) - (p->scale * p->camera.y * height / 2.0));

    // Project the road width onto screen space, scaled based on depth to create perspective.
    p->screen.w = round(p->scale * roadWidth * width / 2.0);
}

// Calculates the percentage (0-1) of n within the total length
double percentRemaining(double n, double total) {
    return fmod(n, total) / total;
}

// Checks if two objects overlap based on their positions and widths.
int overlap(double x1, double w1, double x2, double w2, double percent) {
    // Calculate half of the combined widths, scaled by the overlap threshold percentage.
    double half = (w1 + w2) * (percent / 2.0);

    // Check if the distance between x1 and x2 is less than the calculated half width.
    // If true, it means the objects are within the overlap threshold.
    return fabs(x1 - x2) < half;
}

float lerp_float(float a, float b, float t) {
    return a + t * (b - a);
}

SDL_Surface* blend_surface_with_color(SDL_Surface* surface, SDL_Color blendColor, float blendFactor) {
    if (!surface) {
        fprintf(stderr, "blend_surface_with_color: Received NULL surface.\n");
        return NULL;
    }

    // Ensure blendFactor is within [0.0, 1.0]
    blendFactor = fmaxf(0.0f, fminf(blendFactor, 1.0f));

    // Convert surface to RGBA8888 format for consistent pixel manipulation
    SDL_Surface* convertedSurface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA8888, 0);
    if (!convertedSurface) {
        fprintf(stderr, "blend_surface_with_color: SDL_ConvertSurfaceFormat failed: %s\n", SDL_GetError());
        return NULL;
    }

    // Create a new surface to store the blended result
    SDL_Surface* blendedSurface = SDL_CreateRGBSurfaceWithFormat(0, convertedSurface->w, convertedSurface->h,
                                                                 32, SDL_PIXELFORMAT_RGBA8888);
    if (!blendedSurface) {
        fprintf(stderr, "blend_surface_with_color: SDL_CreateRGBSurfaceWithFormat failed: %s\n", SDL_GetError());
        SDL_FreeSurface(convertedSurface);
        return NULL;
    }

    // Lock both surfaces for direct pixel access
    if (SDL_MUSTLOCK(convertedSurface)) {
        if (SDL_LockSurface(convertedSurface) != 0) {
            fprintf(stderr, "blend_surface_with_color: SDL_LockSurface failed: %s\n", SDL_GetError());
            SDL_FreeSurface(convertedSurface);
            SDL_FreeSurface(blendedSurface);
            return NULL;
        }
    }
    if (SDL_MUSTLOCK(blendedSurface)) {
        if (SDL_LockSurface(blendedSurface) != 0) {
            fprintf(stderr, "blend_surface_with_color: SDL_LockSurface failed: %s\n", SDL_GetError());
            if (SDL_MUSTLOCK(convertedSurface)) SDL_UnlockSurface(convertedSurface);
            SDL_FreeSurface(convertedSurface);
            SDL_FreeSurface(blendedSurface);
            return NULL;
        }
    }

    // Iterate over each pixel and apply blending
    Uint32* srcPixels = (Uint32*)convertedSurface->pixels;
    Uint32* dstPixels = (Uint32*)blendedSurface->pixels;
    int totalPixels = convertedSurface->w * convertedSurface->h;

    for (int i = 0; i < totalPixels; ++i) {
        Uint32 pixel = srcPixels[i];
        Uint8 r, g, b, a;
        SDL_GetRGBA(pixel, convertedSurface->format, &r, &g, &b, &a);

        // Apply blending: blended = original * (1 - blendFactor) + blendColor * blendFactor
        Uint8 blendedR = (Uint8)(r * (1.0f - blendFactor) + blendColor.r * blendFactor);
        Uint8 blendedG = (Uint8)(g * (1.0f - blendFactor) + blendColor.g * blendFactor);
        Uint8 blendedB = (Uint8)(b * (1.0f - blendFactor) + blendColor.b * blendFactor);
        Uint8 blendedA = a; // Preserve original alpha

        dstPixels[i] = SDL_MapRGBA(blendedSurface->format, blendedR, blendedG, blendedB, blendedA);
    }

    // Unlock surfaces if they were locked
    if (SDL_MUSTLOCK(convertedSurface)) {
        SDL_UnlockSurface(convertedSurface);
    }
    if (SDL_MUSTLOCK(blendedSurface)) {
        SDL_UnlockSurface(blendedSurface);
    }

    // Free the converted surface as it's no longer needed
    SDL_FreeSurface(convertedSurface);

    return blendedSurface;
}

SDL_Texture* create_blended_texture(SDL_Renderer* renderer, SDL_Surface* sourceSurface, SDL_Rect srcRect, SDL_Color blendColor, float blendFactor) {
    if (!sourceSurface) {
        fprintf(stderr, "create_blended_texture: Received NULL sourceSurface.\n");
        return NULL;
    }

    // Ensure srcRect is within the bounds of the sourceSurface
    if (srcRect.x < 0 || srcRect.y < 0 ||
        srcRect.x + srcRect.w > sourceSurface->w ||
        srcRect.y + srcRect.h > sourceSurface->h) {
        fprintf(stderr, "create_blended_texture: srcRect is out of bounds.\n");
        return NULL;
    }

    // Create a new surface to hold the extracted area
    SDL_Surface* subSurface = SDL_CreateRGBSurfaceWithFormat(0, srcRect.w, srcRect.h,
                                                             sourceSurface->format->BitsPerPixel,
                                                             sourceSurface->format->format);
    if (!subSurface) {
        fprintf(stderr, "create_blended_texture: SDL_CreateRGBSurfaceWithFormat failed: %s\n", SDL_GetError());
        return NULL;
    }

    // Blit the relevant area from the source surface to the sub-surface
    if (SDL_BlitSurface(sourceSurface, &srcRect, subSurface, NULL) != 0) {
        fprintf(stderr, "create_blended_texture: SDL_BlitSurface failed: %s\n", SDL_GetError());
        SDL_FreeSurface(subSurface);
        return NULL;
    }

    // Apply color blending to the sub-surface
    SDL_Surface* blendedSurface = blend_surface_with_color(subSurface, blendColor, blendFactor);
    if (!blendedSurface) {
        fprintf(stderr, "create_blended_texture: blend_surface_with_color failed.\n");
        SDL_FreeSurface(subSurface);
        return NULL;
    }

    // Create a texture from the blended surface
    SDL_Texture* blendedTexture = SDL_CreateTextureFromSurface(renderer, blendedSurface);
    if (!blendedTexture) {
        fprintf(stderr, "create_blended_texture: SDL_CreateTextureFromSurface failed: %s\n", SDL_GetError());
        SDL_FreeSurface(blendedSurface);
        SDL_FreeSurface(subSurface);
        return NULL;
    }

    // Free temporary surfaces as they are no longer needed
    SDL_FreeSurface(blendedSurface);
    SDL_FreeSurface(subSurface);

    return blendedTexture;
}