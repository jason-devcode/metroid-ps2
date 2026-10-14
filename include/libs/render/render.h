/**
 * @file render.h
 * @brief CPU framebuffer + GPU hardware accelerated rendering library using gsKit for PlayStation 2.
 * @details Fixes framebuffer persistence trails by clearing the CPU buffer alongside hardware presentation.
 */

#ifndef LIB_RENDER_H
#define LIB_RENDER_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <malloc.h>
#include <gsKit.h>
#include <dmaKit.h>

/** @brief Color depth specification in bits per pixel (32-bit ABGR/RGBA). */
#define PIXELS_BPP 32

/**
 * @brief Packs red, green, and blue components into a 32-bit color integer (ABGR format for GS).
 */
#define RGB(r, g, b) (0xFF000000 | (((b & 0xFF) << 16) | ((g & 0xFF) << 8) | ((r & 0xFF) << 0)))

/** @brief Pitch of the screen surface scaled for 32-bit pixel offsets. */
static int screen_pitch = 0;

/** @brief Current display width in pixels. */
static int screen_width = 0;

/** @brief Current display height in pixels. */
static int screen_height = 0;

/** @brief Default color used when clearing the screen (ABGR format for CPU framebuffer). */
static uint32_t clear_screen_color = RGB(25, 25, 25);

/** @brief Default color used when clearing the screen in hardware GS format. */
static uint64_t gs_clear_color = 0;

/** @brief Global flag indicating whether the game/render loop is currently active. */
static bool game_running = false;

/** @brief Direct pointer to the CPU framebuffer pixel buffer. */
static uint32_t *framebuffer = NULL;
static uint32_t *clear_row_buffer = NULL;

// --- Contexto interno de gsKit y DMA ---
static GSGLOBAL *gs_global = NULL;
static GSTEXTURE render_texture;

/**
 * @brief Converts a 32-bit ABGR color to gsKit internal RGBAQ format.
 */
static inline uint64_t pack_gs_color(uint32_t color)
{
    unsigned char r = (color >> 0) & 0xFF;
    unsigned char g = (color >> 8) & 0xFF;
    unsigned char b = (color >> 16) & 0xFF;
    unsigned char a = (color >> 24) & 0xFF;
    return GS_SETREG_RGBAQ(r >> 1, g >> 1, b >> 1, a >> 1, 0x00);
}

/**
 * @brief Initializes the rendering subsystem using gsKit and dmaKit.
 */
static bool init_render(int width, int height)
{
    // 1. Inicializar DMA y canal GIF
    dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC, D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);

    // 2. Inicializar gsKit global
    gs_global = gsKit_init_global();
    if (!gs_global)
    {
        printf("Error initializing gsGlobal\n");
        return false;
    }

    gs_global->Mode = GS_MODE_NTSC;
    gs_global->Width = width;
    gs_global->Height = height;
    gs_global->PSM = GS_PSM_CT32;
    gs_global->DoubleBuffering = GS_SETTING_ON;

    gsKit_init_screen(gs_global);
    gsKit_mode_switch(gs_global, GS_PERSISTENT);

    screen_width = width;
    screen_height = height;
    screen_pitch = width;

    // 3. Crear el framebuffer en memoria de CPU alineado a 64 bytes
    framebuffer = (uint32_t *)memalign(64, width * height * sizeof(uint32_t));
    if (!framebuffer)
    {
        printf("Error allocating framebuffer memory\n");
        return false;
    }

    // 4. Crear una línea de borrado rápido para la CPU optimizada con memcpy
    clear_row_buffer = (uint32_t *)memalign(64, width * sizeof(uint32_t));
    if (clear_row_buffer)
    {
        for (int x = 0; x < width; x++)
        {
            clear_row_buffer[x] = clear_screen_color;
        }
    }

    // 5. Configurar colores de limpieza
    gs_clear_color = pack_gs_color(clear_screen_color);

    // 6. Configurar la textura vinculada al búfer de CPU
    render_texture.Width = width;
    render_texture.Height = height;
    render_texture.PSM = GS_PSM_CT32;
    render_texture.Mem = (u32 *)framebuffer; 
    render_texture.Filter = GS_FILTER_NEAREST;
    render_texture.TBW = (width + 63) / 64;
    render_texture.Vram = gsKit_vram_alloc(gs_global, gsKit_texture_size(width, height, GS_PSM_CT32), 0);

    printf("gsKit hybrid rendering successfully initialized on PS2!\n");

    game_running = true;
    return true;
}

/**
 * @brief Sets a new default background clear color.
 */
static void set_clear_screen_color(uint32_t color) 
{
    clear_screen_color = color;
    gs_clear_color = pack_gs_color(color);

    if (clear_row_buffer && screen_width > 0)
    {
        for (int x = 0; x < screen_width; x++)
        {
            clear_row_buffer[x] = color;
        }
    }
}

/**
 * @brief Clears the CPU-side framebuffer using fast row-by-row memcpy to prevent trails.
 */
static void clear_screen()
{
    if (!framebuffer || !clear_row_buffer) return;

    for (int y = 0; y < screen_height; y++)
    {
        memcpy(&framebuffer[y * screen_width], clear_row_buffer, screen_width * sizeof(uint32_t));
    }
}

/**
 * @brief Clears screen via hardware, uploads texture, renders, and swaps buffers.
 */
static void draw()
{
    if (!gs_global || !game_running) return;

    // 1. Limpiar el búfer oculto en VRAM por hardware
    gsKit_clear(gs_global, gs_clear_color);

    // 2. Subir la textura modificada por la CPU hacia la VRAM
    gsKit_texture_upload(gs_global, &render_texture);
    dmaKit_wait_fast();

    // 3. Renderizar el quad de la textura a pantalla completa por hardware
    gsKit_prim_sprite_texture(gs_global, &render_texture,
                              0.0f, 0.0f,
                              0.0f, 0.0f,
                              (float)screen_width, (float)screen_height,
                              (float)render_texture.Width, (float)render_texture.Height,
                              0,
                              GS_SETREG_RGBAQ(128, 128, 128, 0x80, 0x00));

    // 4. Ejecutar cola de comandos y voltear búferes de hardware sincronizados con VSync
    gsKit_queue_exec(gs_global);
    gsKit_sync_flip(gs_global);
}

/**
 * @brief Shuts down the rendering subsystem and frees allocated memory.
 */
static void close_render()
{
    game_running = false;

    if (framebuffer)
    {
        free(framebuffer);
        framebuffer = NULL;
    }

    if (clear_row_buffer)
    {
        free(clear_row_buffer);
        clear_row_buffer = NULL;
    }

    if (gs_global)
    {
        gsKit_deinit_global(gs_global);
        gs_global = NULL;
    }

    screen_width = 0;
    screen_height = 0;
    screen_pitch = 0;
}

#endif // LIB_RENDER_H