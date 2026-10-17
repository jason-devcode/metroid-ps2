/**
 * @file render.h
 * @brief Batched Quad Hardware-Accelerated Rendering Library using gsKit for PlayStation 2.
 * @details Allows registering multiple textured quads and rendering them in a single draw batch per frame.
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

/** @brief Maximum number of quads that can be batched in a single frame. */
#define MAX_BATCH_QUADS 2048

/**
 * @brief Packs red, green, and blue components into a 32-bit color integer (ABGR format for GS).
 */
#define RGB(r, g, b) (0xFF000000 | (((b & 0xFF) << 16) | ((g & 0xFF) << 8) | ((r & 0xFF) << 0)))

/** @brief Current display width in pixels. */
static int screen_width = 0;

/** @brief Current display height in pixels. */
static int screen_height = 0;

/** @brief Default color used when clearing the screen in hardware GS format. */
static uint64_t gs_clear_color = 0;

/** @brief Global flag indicating whether the game/render loop is currently active. */
static bool game_running = false;

/** @brief Internal gsKit and DMA context. */
static GSGLOBAL *gs_global = NULL;

/**
 * @brief Structure representing a renderable textured quad in the batch system.
 */
typedef struct {
    float x, y;             /**< Screen position coordinates. */
    float w, h;             /**< Screen width and height. */
    float u1, v1, u2, v2;   /**< Texture UV coordinates. */
    uint32_t color;         /**< Tint color in ABGR format. */
    GSTEXTURE *texture;     /**< Pointer to the texture to use. */
} RenderQuad;

/** @brief Static quad buffer for the current frame's batched draw call. */
static RenderQuad quad_batch[MAX_BATCH_QUADS];
static int quad_batch_count = 0;

/**
 * @brief Converts a 32-bit ABGR color to gsKit internal RGBAQ format.
 * @param color The 32-bit ABGR color value.
 * @return The converted 64-bit GS color register value.
 */
static inline uint64_t pack_gs_color(uint32_t color)
{
    unsigned char r = (color >> 0) & 0xFF;
    unsigned char g = (color >> 8) & 0xFF;
    unsigned char b = (color >> 16) & 0xFF;
    unsigned char a = (color >> 24) & 0xFF;

    /* RGB retains its 8 bits; alpha is scaled to the 0-128 range. */
    unsigned char gs_alpha = (unsigned char)((a * 128U + 127U) / 255U);

    return GS_SETREG_RGBAQ(r, g, b, gs_alpha, 0x00);
}

/**
 * @brief Initializes the rendering subsystem using gsKit and dmaKit for batched quad rendering.
 * @param width Display width in pixels.
 * @param height Display height in pixels.
 * @return True if initialization succeeds, false otherwise.
 */
static bool init_render(int width, int height)
{
    dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC, D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);

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

    /* Enable alpha blending usage in primitives and textures. */
    gs_global->PrimAlphaEnable = GS_SETTING_ON;

    gsKit_init_screen(gs_global);
    gsKit_mode_switch(gs_global, GS_PERSISTENT);

    screen_width = width;
    screen_height = height;
    quad_batch_count = 0;

    gs_clear_color = pack_gs_color(RGB(25, 25, 25));

    printf("gsKit Batched Quad rendering successfully initialized on PS2!\n");

    game_running = true;
    return true;
}

/**
 * @brief Sets a new default background clear color.
 * @param color The new color in 32-bit ABGR format.
 */
static void set_clear_screen_color(uint32_t color) 
{
    gs_clear_color = pack_gs_color(color);
}

/**
 * @brief Creates and allocates VRAM for a custom texture (e.g., procedural shapes).
 * @param texture Pointer to the GSTEXTURE structure.
 * @param width Texture width in pixels.
 * @param height Texture height in pixels.
 * @param pixels Pointer to the source pixel data.
 * @return True if creation and VRAM allocation succeed, false otherwise.
 */
static bool create_texture(GSTEXTURE *texture, int width, int height, uint32_t *pixels)
{
    texture->Width = width;
    texture->Height = height;
    texture->PSM = GS_PSM_CT32;
    texture->Mem = (u32 *)pixels; 
    texture->Filter = GS_FILTER_NEAREST;
    texture->TBW = (width + 63) / 64;
    
    texture->Vram = gsKit_vram_alloc(gs_global, gsKit_texture_size(width, height, GS_PSM_CT32), GSKIT_ALLOC_USERBUFFER);
    
    if (texture->Vram == GSKIT_ALLOC_ERROR) {
        printf("Error allocating VRAM for texture!\n");
        return false;
    }

    gsKit_texture_upload(gs_global, texture);
    dmaKit_wait_fast();
    return true;
}

/**
 * @brief Crea y asigna VRAM para una textura CI8 (T8) nativa con su paleta (CLUT).
 * @param texture Puntero a la estructura GSTEXTURE.
 * @param width Ancho de la textura en píxeles.
 * @param height Alto de la textura en píxeles.
 * @param indices Puntero al array de índices de 1 byte.
 * @param palette Puntero al array de colores de la paleta en formato ABGR (32-bit).
 * @return True si la asignación y subida tienen éxito, false en caso contrario.
 */
static bool create_ci8_texture(GSTEXTURE *texture, int width, int height, const uint8_t *indices, const uint32_t *palette)
{
    memset(texture, 0, sizeof(GSTEXTURE));

    texture->Width = width;
    texture->Height = height;
    texture->PSM = GS_PSM_T8;          // Formato indexado de 8 bits (1 byte por píxel)
    texture->ClutPSM = GS_PSM_CT32;    // Paleta en formato de color real de 32 bits (ABGR)
    texture->TBW = (width + 63) / 64;  // Ancho de búfer de textura para T8
    
    // Asignar punteros en memoria de EE
    texture->Mem = (u32 *)indices;
    texture->Clut = (u32 *)palette;
    
    texture->Filter = GS_FILTER_NEAREST; // Filtro Nearest para evitar interpolación borrosa/con ruido
    texture->ClutStorageMode = GS_CLUT_STORAGE_CSM1; // Modo de almacenamiento CSM1 estándar para evitar corrupción de paleta en T8
    texture->Delayed = 0;

    // 1. Asignar VRAM para los índices de la textura T8
    texture->Vram = gsKit_vram_alloc(gs_global, gsKit_texture_size(width, height, GS_PSM_T8), GSKIT_ALLOC_USERBUFFER);
    if (texture->Vram == GSKIT_ALLOC_ERROR) {
        printf("Error al asignar VRAM para la textura CI8!\n");
        return false;
    }

    // 2. Asignar VRAM para la CLUT (bloque de 16x16 en CT32)
    texture->VramClut = gsKit_vram_alloc(gs_global, gsKit_texture_size(16, 16, GS_PSM_CT32), GSKIT_ALLOC_USERBUFFER);
    if (texture->VramClut == GSKIT_ALLOC_ERROR) {
        printf("Error al asignar VRAM para la CLUT de la textura CI8!\n");
        return false;
    }

    // 3. Subir índices y paleta al GS
    gsKit_texture_upload(gs_global, texture);
    dmaKit_wait_fast();

    return true;
}

/**
 * @brief Registers a textured quad to be drawn in the current frame batch.
 * @param x X coordinate on screen.
 * @param y Y coordinate on screen.
 * @param w Width on screen.
 * @param h Height on screen.
 * @param color Tint color in 32-bit ABGR format.
 * @param texture Pointer to the texture to apply.
 */
static void render_quad(float x, float y, float w, float h, float u1, float v1, float u2, float v2, uint32_t color, GSTEXTURE *texture)
{
    if (quad_batch_count >= MAX_BATCH_QUADS) return;

    RenderQuad *q = &quad_batch[quad_batch_count++];
    q->x = x;
    q->y = y;
    q->w = w;
    q->h = h;
    q->u1 = u1;
    q->v1 = v1;
    q->u2 = u2;
    q->v2 = v2;
    q->color = color;
    q->texture = texture;
}

/**
 * @brief Clears the frame batch list at the beginning of a new frame.
 */
static void begin_frame()
{
    quad_batch_count = 0;
}

/**
 * @brief Clears screen via hardware, flushes and draws all registered quads in a single batch, and swaps buffers.
 */
static void draw()
{
    if (!gs_global || !game_running) return;

    gsKit_queue_reset(gs_global->CurQueue);

    /* 1. Clear the hidden buffer in VRAM via hardware. */
    gsKit_clear(gs_global, gs_clear_color);

    /* Clean and direct configuration for alpha channel without restrictive tests that could cause freezes. */
    gsKit_set_primalpha(
        gs_global,
        GS_SETREG_ALPHA(0, 1, 0, 1, 0),
        0
    );

    /* 2. Draw all quads registered in the batch with a single logical call. */
    for (int i = 0; i < quad_batch_count; i++)
    {
        RenderQuad *q = &quad_batch[i];
        
        gsKit_prim_sprite_texture(gs_global, q->texture,
                                  q->x, q->y,
                                  q->u1, q->v1,
                                  q->x + q->w, q->y + q->h,
                                  q->u2, q->v2,
                                  0,
                                  pack_gs_color(q->color));
    }

    /* 3. Execute command queue and swap hardware buffers synchronized with VSync. */
    gsKit_queue_exec(gs_global);
    gsKit_sync_flip(gs_global);
}

/**
 * @brief Shuts down the rendering subsystem and frees resources.
 */
static void close_render()
{
    game_running = false;

    if (gs_global)
    {
        gsKit_deinit_global(gs_global);
        gs_global = NULL;
    }

    screen_width = 0;
    screen_height = 0;
}

#endif // LIB_RENDER_H