/**
 * @file shapes.h
 * @brief Geometric shape render management system for PS2 using gsKit.
 * 
 * This module provides a memory arena and command submission pipeline for 
 * rendering 2D geometric shapes efficiently per frame.
 */

#ifndef LIB_SHAPES_RENDER_MANAGEMENT_H
#define LIB_SHAPES_RENDER_MANAGEMENT_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <gsKit.h>
#include <libs/math/vectorial.h>

/* ============================================================================
 * CONFIGURATION CONSTANTS & MACROS
 * ============================================================================ */

/** @brief Size in bytes of a single memory arena chunk for alignment. */
#define SHAPE_MEMORY_CHUNK_SIZE 4

/** @brief Maximum number of chunks available in the memory arena. */
#define SHAPE_MAX_MEMORY_CHUNKS 256

/** @brief Total size of the shape memory arena in bytes (1024 bytes). */
#define SHAPE_MEMORY_ARENA_SIZE (SHAPE_MEMORY_CHUNK_SIZE * SHAPE_MAX_MEMORY_CHUNKS)

/** @brief Maximum number of distinct shapes that can be registered per frame. */
#define MAX_SHAPES_PER_FRAME 1024

/* ============================================================================
 * TYPE DEFINITIONS
 * ============================================================================ */

/** @brief Byte data type alias. */
typedef int8_t byte;

/** @brief Identifier type for classifying different geometric shape types. */
typedef int8_t ShapeType;

/**
 * @brief Represents a generic shape allocation entry inside the command buffer.
 */
typedef struct __tag_shape_memory_allocation__
{
    void *shape_frame_data; /**< Pointer to the shape's specific data in the memory arena. */
    ShapeType type;         /**< The type classification of the registered shape. */
} Shape;

/**
 * @brief Geometric contract structure defining a line shape's memory layout.
 */
typedef struct __tag_shape_line_contract__
{
    vec2f A;         /**< Starting 2D point of the line. */
    vec2f B;         /**< Ending 2D point of the line. */
    uint32_t color;  /**< Packed color of the line. */
} ShapeLine;

/** @brief Macro evaluating the exact memory footprint of a ShapeLine structure. */
#define SHAPE_STRUCT_LINE_SIZE sizeof(ShapeLine)

/**
 * @brief Supported shape type enumeration identifiers.
 */
enum SHAPE_TYPE
{
    GS_LINE,                       /**< Simple line primitive type. */
    GS_SHAPE_DRAW_FUNCTIONS_COUNT  /**< Total count of shape types used for array sizing. */
};

/** @brief Function pointer signature for drawing a registered shape. */
typedef void (*ShapeDrawFunction)(GSGLOBAL* gs_global, void* shape_data);

/* ============================================================================
 * GLOBAL STATE VARIABLES
 * ============================================================================ */

/** @brief Linear memory arena buffer storing frame-specific shape data. */
static byte shapes_memory_arena[SHAPE_MEMORY_ARENA_SIZE] = {0};

/** @brief Tracks the total number of bytes currently allocated in the memory arena. */
static uint32_t shape_memory_allocated = 0;

/** @brief Command submission array holding all active shapes for the current frame. */
static Shape shapes[MAX_SHAPES_PER_FRAME] = {0};

/** @brief Counter tracking how many shapes have been registered in the current frame. */
static int32_t shape_counter = 0;

/** @brief Function pointer table mapping shape types to their respective draw implementations. */
static ShapeDrawFunction shape_functions_draw_calls[GS_SHAPE_DRAW_FUNCTIONS_COUNT];

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

/**
 * @brief Packs a standard color into a gsKit-compatible format.
 * @param color The raw 32-bit color value.
 * @return uint64_t The GS-compatible packed color.
 */
static inline uint64_t pack_gs_color(uint32_t color);

/* ============================================================================
 * MEMORY MANAGEMENT FUNCTIONS
 * ============================================================================ */

/**
 * @brief Resets the shape memory arena and counters for a new frame.
 */
static inline void reset_shape_memory_arena(void)
{
    shape_memory_allocated = 0;
    shape_counter = 0;
}

/**
 * @brief Aligns a given byte size to the nearest memory arena chunk multiple
 *        for geometric shape data structures to be rendered.
 *
 * @details **1. Mathematical Foundation (The Stepped Behavior):**
 *          The core behavior relies on a step function designed to round values
 *          up to the nearest multiple of 4 bytes:
 *          \f[ f(size) = \lfloor \frac{size + 3}{4} \rfloor \times 4 \f]
 *          Adding a bias of 3 shifts the boundaries so that any non-zero remainder
 *          pushes the value into the next integer step (ascending staircase pattern).
 *          Sizes from 1 to 4 map to 4; sizes from 5 to 8 map to 8, and exact multiples
 *          sit right on the edge, keeping their values without unnecessary over-padding.
 *
 *          **2. Transition to the Efficient Bitwise Calculation:**
 *          Executing division and multiplication at the CPU level can be computationally
 *          expensive. Since 4 is a power of 2 (2^2), division by 4 is equivalent to
 *          shifting bits 2 positions to the right (`>> 2`), and multiplication is shifting
 *          left (`<< 2`).
 *          Instead of shifting back and forth, we use a bitwise mask (`& ~3`):
 *          - The number `3` in binary is `0000...0011`.
 *          - Its bitwise negation (`~3`) is `1111...1100`, acting as a mask that forces
 *            the last 2 bits to zero (`00`).
 *          - Any binary number whose last two bits are `00` is an exact multiple of 4.
 *          Combining the bias `(size + 3)` with the `& ~3` mask clears the remainder
 *          bits instantly in a single CPU cycle, bypassing heavy division instructions
 *          while perfectly preserving the alignment logic.
 *
 * @param size The raw size in bytes requested for the shape data.
 * @return int32_t The aligned size rounded up to the nearest multiple of 4 bytes.
 */
static inline int32_t calculate_aligned_shape_memory_size(int32_t size)
{
    return (size + SHAPE_MEMORY_CHUNK_SIZE - 1) & ~3;
}

/**
 * @brief Allocates a block of memory inside the arena for a shape's frame data.
 * 
 * @param[out] data Double pointer receiving the allocated memory address.
 * @param[in] size Requested size in bytes.
 * @return true If allocation succeeded.
 * @return false If arena capacity was exceeded.
 */
static inline bool allocate_shape_memory_arena(void **data, int32_t size)
{
    if (!data)
        return false;

    int aligned_size = calculate_aligned_shape_memory_size(size);
    int new_mem_address = shape_memory_allocated + aligned_size;

    if (SHAPE_MEMORY_ARENA_SIZE < new_mem_address)
        return false;

    *data = &shapes_memory_arena[shape_memory_allocated];
    shape_memory_allocated = new_mem_address;

    return true;
}

/**
 * @brief Registers a shape to be drawn during the current frame rendering pass.
 * 
 * @param type The type identifier of the shape.
 * @param data Pointer to the shape data source to copy from.
 * @param size Size in bytes of the shape data structure.
 */
static inline void register_shape_to_draw(ShapeType type, void *data, int32_t size)
{
    if (!data || size <= 0)
        return;

    Shape *current_shape = &shapes[shape_counter];

    if (!allocate_shape_memory_arena(&current_shape->shape_frame_data, size))
        return;

    current_shape->type = type;

    // Copy shape data securely into the memory arena block using standard memcpy
    memcpy(current_shape->shape_frame_data, data, size);

    ++shape_counter;
}

/* ============================================================================
 * DRAW CALL IMPLEMENTATIONS
 * ============================================================================ */

/**
 * @brief Draw call implementation for rendering a simple line primitive via gsKit.
 * 
 * @param gs_global Pointer to the gsKit global context.
 * @param line_data Pointer to the ShapeLine structure containing coordinates and color.
 */
static void shape_draw_call_simple_line(GSGLOBAL* gs_global, ShapeLine* line_data)
{
    gsKit_prim_line(
        gs_global,
        line_data->A.x, line_data->A.y,
        line_data->B.x, line_data->B.y, 0,
        pack_gs_color(line_data->color)
    );
}

/* ============================================================================
 * PUBLIC INTERFACE & SYSTEM FUNCTIONS
 * ============================================================================ */

/**
 * @brief Submits a line shape to the render queue for the current frame.
 * 
 * @param line_data The ShapeLine structure containing line endpoints and color.
 */
static inline void render_line_shape(const ShapeLine line_data)
{
    register_shape_to_draw(GS_LINE, (void*)&line_data, SHAPE_STRUCT_LINE_SIZE);
}

/**
 * @brief Initializes the shape rendering system and binds draw call handlers.
 */
static inline void initialize_draw_shapes_system(void)
{
    shape_functions_draw_calls[GS_LINE] = (ShapeDrawFunction)shape_draw_call_simple_line;
}

/**
 * @brief Iterates through all registered shapes and executes their draw calls.
 * 
 * @param gs_global Pointer to the gsKit global context.
 */
static inline void render_all_shapes(GSGLOBAL* gs_global)
{
    for (int i = 0; i < shape_counter; ++i)
    {
        Shape *current_shape = &shapes[i];
        
        ShapeDrawFunction shape_draw_function = shape_functions_draw_calls[current_shape->type];

        if (!shape_draw_function) 
            continue;
        
        void *draw_shape_data = current_shape->shape_frame_data;

        shape_draw_function(gs_global, draw_shape_data);
    }
}

#endif // LIB_SHAPES_RENDER_MANAGEMENT_H