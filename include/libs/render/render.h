/**
 * @file lib_render.h
 * @brief Hardware-accelerated rendering and display management library using SDL1 for the PlayStation 2.
 * @details Configured for SDL_HWSURFACE to leverage PS2 VRAM, providing a direct framebuffer pointer without surface locks.
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

/** @brief Default color used when clearing the screen. */
uint32_t clear_screen_color = RGB(15, 25, 45);

/** @brief Global flag indicating whether the game/render loop is currently active. */
bool game_running = false;

/** @brief Direct pointer to the hardware surface pixel buffer (framebuffer). */
uint32_t *framebuffer = NULL;

/** @brief Main SDL video surface handle. */
SDL_Surface *screen_surface = NULL;

/**
 * @brief Initializes the rendering subsystem with hardware acceleration.
 * @param width Desired screen width in pixels.
 * @param height Desired screen height in pixels.
 * @return `true` if initialization succeeded, `false` otherwise.
 */
static bool init_render(int width, int height)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("Error initializing SDL: %s\n", SDL_GetError());
        return false;
    }

    screen_width = width;
    screen_height = height;

    // Modo de video por hardware con doble búfer
    screen_surface = SDL_SetVideoMode(width, height, PIXELS_BPP, SDL_HWSURFACE | SDL_DOUBLEBUF);

    if (!screen_surface)
    {
        printf("Error setting hardware video mode: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    framebuffer = (uint32_t *)screen_surface->pixels;
    screen_pitch = screen_surface->pitch / 4;

    SDL_ShowCursor(SDL_DISABLE);

    printf("SDL1 hardware rendering successfully initialized on PS2!\n");

    game_running = true;

    return true;
}

/**
 * @brief Sets a new default background clear color.
 * @param color Packed 32-bit color value.
 */
static void set_clear_screen_color(uint32_t color) 
{
    clear_screen_color = color;
}

/**
 * @brief Clears the hardware back buffer using the current clear color.
 */
static void clear_screen()
{
    SDL_FillRect(screen_surface, NULL, clear_screen_color);
}

/**
 * @brief Swaps hardware display buffers to present the rendered frame.
 */
static void draw()
{
    // Actualizar el puntero del framebuffer por seguridad en cada frame
    framebuffer = (uint32_t *)screen_surface->pixels;
    
    SDL_Flip(screen_surface);
}

/**
 * @brief Shuts down the rendering subsystem and quits SDL.
 */
static void close_render()
{
    game_running = false;
    screen_width = 0;
    screen_height = 0;
    screen_pitch = 0;
    screen_surface = NULL;
    framebuffer = NULL;
    SDL_Quit();
}

#endif // LIB_RENDER_H