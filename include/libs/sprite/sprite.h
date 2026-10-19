/**
 * @file sprite.h
 * @brief Sprite atlas structure definition and interface for sprite sheet rendering.
 * @details Encapsulates gsKit textures and grid metadata for fast animation frame 
 *          calculations on the Emotion Engine (EE) CPU.
 */

#ifndef LIB_SPRITE_H
#define LIB_SPRITE_H

#include <gsKit.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>

#include <libs/texture/texture.h>

/**
 * @struct __tag_sprite_structure__
 * @brief Structure representing an immutable sprite atlas (sprite sheet) for rendering.
 * 
 * This structure encapsulates a gsKit texture alongside precalculated grid metadata
 * used for splitting the source image into animation frames or sub-cells. All fields are
 * native 32-bit registers to eliminate compiler bitmasking overhead during frame calculation loop.
 * 
 * @note The total size of this structure is strictly guaranteed to be 56 bytes to maintain 
 * perfect memory alignment and high-throughput cache access on the Emotion Engine (EE) CPU.
 */
typedef struct __tag_sprite_structure__ 
{
    /** @brief The underlying gsKit texture structure containing VRAM pointers and format data. */
    Texture texture;     
    
    /** @brief Total number of horizontal columns within the sprite sheet grid. */
    uint32_t cols;          
    
    /** @brief Total number of vertical rows within the sprite sheet grid. */
    uint32_t rows;          
    
    /** @brief Pixel width of a single individual animation cell/frame. */
    uint32_t cell_width;   
    
    /** @brief Pixel height of a single individual animation cell/frame. */
    uint32_t cell_height;  
} Sprite;

/**
 * @brief Compile-time validation for structural size alignment.
 * 
 * Ensures that the compiler layout matches exactly 56 bytes (40 bytes for GSTEXTURE + 16 bytes for 
 * control metadata) under extreme optimization flags (-O3, -flto) in the metroid-ps2 environment.
 */
static_assert(sizeof(Sprite) == 56, "Error! Unexpected padding or structure size alteration detected in Sprite layout.");

/**
 * @brief Initializes a Sprite atlas and uploads raw 32-bit (RGBA/ABGR) pixel data to VRAM.
 * 
 * @param sprite Pointer to the target Sprite structure.
 * @param width Full image width in pixels.
 * @param height Full image height in pixels.
 * @param cols Number of columns in the sprite sheet grid.
 * @param rows Number of rows in the sprite sheet grid.
 * @param tile_width Width of an individual frame/tile in pixels.
 * @param tile_height Height of an individual frame/tile in pixels.
 * @param pixels Pointer to 32-bit pixel data array.
 * 
 * @return True if sprite initialization and VRAM allocation succeed, false otherwise.
 */
static bool create_sprite(Sprite *sprite, int width, int height, uint32_t cols, uint32_t rows, uint32_t tile_width, uint32_t tile_height, uint32_t *pixels)
{
    if (!sprite || cols == 0 || rows == 0) return false;

    sprite->cols = cols;
    sprite->rows = rows;
    sprite->cell_width = tile_width;
    sprite->cell_height = tile_height;

    return create_texture(&sprite->texture, width, height, pixels);
}

/**
 * @brief Initializes a Sprite atlas and uploads 8-bit indexed (CI8) image data and palette to VRAM.
 * 
 * @param sprite Pointer to the target Sprite structure.
 * @param width Full image width in pixels.
 * @param height Full image height in pixels.
 * @param cols Number of columns in the sprite sheet grid.
 * @param rows Number of rows in the sprite sheet grid.
 * @param tile_width Width of an individual frame/tile in pixels.
 * @param tile_height Height of an individual frame/tile in pixels.
 * @param indices Pointer to 8-bit index array (1 byte per pixel).
 * @param palette Pointer to 32-bit palette color array (CLUT).
 * 
 * @return True if sprite allocation and VRAM upload succeed, false otherwise.
 */
static bool create_ci8_sprite(Sprite *sprite, int width, int height, uint32_t cols, uint32_t rows, uint32_t tile_width, uint32_t tile_height, const uint8_t *indices, const uint32_t *palette)
{
    if (!sprite || cols == 0 || rows == 0) return false;

    sprite->cols = cols;
    sprite->rows = rows;
    sprite->cell_width = tile_width;
    sprite->cell_height = tile_height;

    return create_ci8_texture(&sprite->texture, width, height, indices, palette);
}

#endif // LIB_SPRITE_H