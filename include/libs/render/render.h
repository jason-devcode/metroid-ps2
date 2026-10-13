/**
 * @file lib_render.h
 * @brief Software rendering and display management library using SDL1 for the PlayStation 2.
 * @details Provides core rendering state variables, framebuffer management, screen clearing, 
 *          and surface flipping functions tailored for retro game development loops.
 */

#ifndef LIB_RENDER_H
#define LIB_RENDER_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <SDL.h>

/** @brief Color depth specification in bits per pixel (32-bit RGBA/XRGB). */
#define PIXELS_BPP 32

/**
 * @brief Packs red, green, and blue components into a 32-bit color integer.
 * 
 * Forces alpha to fully opaque (0xFF).
 * 
 * @param r Red component (0-255).
 * @param g Green component (0-255).
 * @param b Blue component (0-255).
 * @return Packed 32-bit color value (`uint32_t`).
 */
#define RGB(r, g, b) (0xFF000000 | (((b & 0xFF) << 16) | ((g & 0xFF) << 8) | ((r & 0xFF) << 0)))

/** @brief Pitch of the screen surface scaled for 32-bit pixel offsets. */
int screen_pitch = 0;

/** @brief Current display width in pixels. */
int screen_width = 0;

/** @brief Current display height in pixels. */
int screen_height = 0;

/** @brief Default color used when clearing the screen (defaults to a dark blue tone). */
uint32_t clear_screen_color = RGB(15, 25, 45);

/** @brief Global flag indicating whether the game/render loop is currently active. */
bool game_running = false;

/** @brief Direct pointer to the SDL surface pixel buffer (framebuffer). */
uint32_t *framebuffer = NULL;

/** @brief Main SDL video surface handle. */
SDL_Surface *screen_surface = NULL;

/**
 * @brief Initializes the rendering subsystem and sets up the video mode.
 * 
 * @param width Desired screen width in pixels.
 * @param height Desired screen height in pixels.
 * @return `true` if initialization succeeded, `false` otherwise.
 */
static bool init_render(int width, int height)
{
    // Initialize SDL with video support
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("Error initializing SDL: %s\n", SDL_GetError());
        return false;
    }

    screen_width = width;
    screen_height = height;
    screen_surface = SDL_SetVideoMode(width, height, PIXELS_BPP, SDL_SWSURFACE);
    framebuffer = screen_surface->pixels;

    if (!screen_surface)
    {
        printf("Error setting video mode: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    screen_pitch = screen_surface->pitch >> 2;

    SDL_ShowCursor(SDL_DISABLE);

    printf("SDL1 successfully initialized on PS2!\n");

    game_running = true;

    return true;
}

/**
 * @brief Sets a new default background clear color.
 * 
 * @param color Packed 32-bit color value (use the @ref RGB macro).
 */
static void set_clear_screen_color(uint32_t color) 
{
    clear_screen_color = color;
}

/**
 * @brief Clears the back buffer/screen surface using the current clear color.
 */
static void clear_screen()
{
    SDL_FillRect(screen_surface, NULL, clear_screen_color);
}

/**
 * @brief Swaps/updates the display buffers to present the rendered frame on screen.
 */
static void draw()
{
    SDL_Flip(screen_surface);
}

/**
 * @brief Shuts down the rendering subsystem, resets variables, and quits SDL.
 */
static void close_render()
{
    game_running = false;
    screen_width = 0;
    screen_height = 0;
    screen_pitch = 0;
    screen_surface = NULL;
    SDL_Quit();
}

#endif // LIB_RENDER_H