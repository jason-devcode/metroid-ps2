/**
 * @file lib_sprite_animation.h
 * @brief Provides structures and functions for managing and updating sprite animations.
 */

#ifndef LIB_SPRITE_ANIMATION_H
#define LIB_SPRITE_ANIMATION_H

#include <stdbool.h>
#include <math.h>
#include <libs/sprite/sprite.h>
#include <libs/performance/fps.h>

/**
 * @brief Structure representing a sprite animation instance.
 */
typedef struct __tag_sprite_animation_structure__
{
    Sprite *sprite;       /**< Pointer to the base sprite sheet instance. */
    float frame_speed;    /**< Speed at which frames advance horizontally. */
    float frame_index;    /**< Current active frame index. */
    float frame_start;    /**< Starting frame index of the animation sequence. */
    float frame_end;      /**< Ending frame index of the animation sequence. */
    uint32_t row_index;   /**< Row index on the sprite sheet. */
    bool flip_h;          /**< Flag to flip the texture horizontally. */
    bool flip_v;          /**< Flag to flip the texture vertically. */
    bool loop;            /**< Flag indicating whether the animation should loop. */
} SpriteAnimation;

/**
 * @brief Structure representing the calculated texel coordinates for the current frame.
 */
typedef struct __tag_sprite_animation_frame_texel_coordinates__
{
    uint32_t u1; /**< Top-left horizontal texture coordinate. */
    uint32_t v1; /**< Top-left vertical texture coordinate. */
    uint32_t u2; /**< Bottom-right horizontal texture coordinate. */
    uint32_t v2; /**< Bottom-right vertical texture coordinate. */
} SpriteFrame;

/**
 * @brief Initializes a sprite animation instance with specified parameters.
 * 
 * @param animation Pointer to the SpriteAnimation structure to initialize.
 * @param sprite Pointer to the base Sprite sheet.
 * @param frame_speed Playback speed of the animation.
 * @param frame_index Initial frame index.
 * @param frame_start Starting frame index.
 * @param frame_end Ending frame index.
 * @param row_index Row index on the sprite sheet.
 * @param flip_h Horizontal flip flag.
 * @param flip_v Vertical flip flag.
 * @param loop Loop flag.
 * @return true if initialization was successful, false if pointers were NULL.
 */
static inline bool create_sprite_animation(
    SpriteAnimation *animation,
    Sprite *sprite,
    float frame_speed,
    float frame_index,
    float frame_start,
    float frame_end,
    uint32_t row_index,
    bool flip_h,
    bool flip_v,
    bool loop)
{
    if (!animation || !sprite)
    {
        return false;
    }

    animation->sprite = sprite;
    animation->frame_speed = frame_speed;
    animation->frame_index = frame_index;
    animation->frame_start = frame_start;
    animation->frame_end = frame_end;
    animation->row_index = row_index;
    animation->flip_h = flip_h;
    animation->flip_v = flip_v;
    animation->loop = loop;

    return true;
}

/**
 * @brief Updates the sprite animation state and calculates current frame texel coordinates.
 * 
 * @param animation Pointer to the SpriteAnimation instance to update.
 * @param current_frame Pointer to the SpriteFrame structure to populate with coordinates.
 */
static inline void update_sprite_animation(SpriteAnimation *animation, SpriteFrame *current_frame)
{
    if (!animation || !current_frame || !animation->sprite)
        return;

    // Cache cell dimensions as integers to avoid floating-point overhead in coordinate multiplications
    const uint32_t cell_width = animation->sprite->cell_width;
    const uint32_t cell_height = animation->sprite->cell_height;

    // Calculate horizontal texture coordinates using optimized floorf
    const uint32_t current_floor = (uint32_t)floorf(animation->frame_index);
    current_frame->u1 = current_floor * cell_width;
    current_frame->u2 = (current_floor + 1U) * cell_width;

    if (animation->flip_h)
    {
        const uint32_t temp_u = current_frame->u1;
        current_frame->u1 = current_frame->u2;
        current_frame->u2 = temp_u;
    }

    // Calculate vertical texture coordinates based on the row index
    current_frame->v1 = animation->row_index * cell_height;
    current_frame->v2 = (animation->row_index + 1U) * cell_height;

    if (animation->flip_v)
    {
        const uint32_t temp_v = current_frame->v1;
        current_frame->v1 = current_frame->v2;
        current_frame->v2 = temp_v;
    }

    // Advance frame index and handle loop bounds
    animation->frame_index += animation->frame_speed;
    if (animation->frame_index > animation->frame_end)
    {
        animation->frame_index = animation->loop ? animation->frame_start : animation->frame_end;
    }
}

#endif // LIB_SPRITE_ANIMATION_H