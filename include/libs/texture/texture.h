/**
 * @file texture.h
 * @brief Utility functions for texture creation and VRAM management using gsKit.
 * @details Provides helper functions to initialize, allocate VRAM, and upload standard and indexed (CI8) textures for the PlayStation 2.
 */

#ifndef LIB_TEXTURE_H
#define LIB_TEXTURE_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <gsKit.h>
#include <dmaKit.h>
#include <tamtypes.h>

/** @brief Alias for the underlying gsKit texture structure. */
typedef GSTEXTURE Texture;

/**
 * @brief Creates and allocates VRAM for a custom texture (e.g., procedural shapes).
 * 
 * @param texture Pointer to the Texture structure.
 * @param width Texture width in pixels.
 * @param height Texture height in pixels.
 * @param pixels Pointer to the source pixel data.
 * @return True if creation and VRAM allocation succeed, false otherwise.
 */
static bool create_texture(Texture *texture, int width, int height, uint32_t *pixels)
{
    texture->Width = width;
    texture->Height = height;
    texture->PSM = GS_PSM_CT32;
    texture->Mem = (u32 *)pixels;
    texture->Filter = GS_FILTER_NEAREST;
    texture->TBW = (width + 63) / 64;

    texture->Vram = gsKit_vram_alloc(gs_global, gsKit_texture_size(width, height, GS_PSM_CT32), GSKIT_ALLOC_USERBUFFER);

    if (texture->Vram == GSKIT_ALLOC_ERROR)
    {
        printf("Error allocating VRAM for texture!\n");
        return false;
    }

    gsKit_texture_upload(gs_global, texture);
    dmaKit_wait_fast();
    return true;
}

/**
 * @brief Creates and allocates VRAM for a native CI8 (T8) texture with its palette (CLUT).
 * 
 * @param texture Pointer to the Texture structure.
 * @param width Texture width in pixels.
 * @param height Texture height in pixels.
 * @param indices Pointer to the 1-byte index array.
 * @param palette Pointer to the palette color array in ABGR format (32-bit).
 * @return True if allocation and upload succeed, false otherwise.
 */
static bool create_ci8_texture(Texture *texture, int width, int height, const uint8_t *indices, const uint32_t *palette)
{
    memset(texture, 0, sizeof(Texture));

    texture->Width = width;
    texture->Height = height;
    texture->PSM = GS_PSM_T8;         // 8-bit indexed format (1 byte per pixel)
    texture->ClutPSM = GS_PSM_CT32;   // Palette in 32-bit true color format (ABGR)
    texture->TBW = (width + 63) / 64; // Texture buffer width for T8

    // Assign pointers in EE memory
    texture->Mem = (u32 *)indices;
    texture->Clut = (u32 *)palette;

    texture->Filter = GS_FILTER_NEAREST;             // Nearest filter to avoid blurry/noisy interpolation
    texture->ClutStorageMode = GS_CLUT_STORAGE_CSM1; // Standard CSM1 storage mode to prevent T8 palette corruption
    texture->Delayed = 0;

    // 1. Allocate VRAM for T8 texture indices
    texture->Vram = gsKit_vram_alloc(gs_global, gsKit_texture_size(width, height, GS_PSM_T8), GSKIT_ALLOC_USERBUFFER);
    if (texture->Vram == GSKIT_ALLOC_ERROR)
    {
        printf("Error allocating VRAM for CI8 texture!\n");
        return false;
    }

    // 2. Allocate VRAM for the CLUT (16x16 block in CT32)
    texture->VramClut = gsKit_vram_alloc(gs_global, gsKit_texture_size(16, 16, GS_PSM_CT32), GSKIT_ALLOC_USERBUFFER);
    if (texture->VramClut == GSKIT_ALLOC_ERROR)
    {
        printf("Error allocating VRAM for CI8 texture CLUT!\n");
        return false;
    }

    // 3. Upload indices and palette to the GS
    gsKit_texture_upload(gs_global, texture);
    dmaKit_wait_fast();

    return true;
}

#endif // LIB_TEXTURE_H