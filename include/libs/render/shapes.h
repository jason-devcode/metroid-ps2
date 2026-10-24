#ifndef LIB_SHAPES_RENDER_MANAGEMENT_H
#define LIB_SHAPES_RENDER_MANAGEMENT_H

#include <stdint.h>
#include <stdbool.h>

#include <libs/math/vectorial.h>

typedef int8_t byte;

#define SHAPE_MEMORY_CHUNK_SIZE 4
#define SHAPE_MAX_MEMORY_CHUNKS 256
#define SHAPE_MEMORY_ARENA_SIZE (SHAPE_MEMORY_CHUNK_SIZE * SHAPE_MAX_MEMORY_CHUNKS)

#define MAX_SHAPES_PER_FRAME 1024

byte shapes_memory_arena[SHAPE_MEMORY_ARENA_SIZE] = {0};

uint32_t shape_memory_allocated = 0;

typedef int8_t ShapeType;

typedef struct __tag_shape_memory_allocation__
{
    void *shape_frame_data;
    ShapeType type;
} Shape;

Shape shapes[MAX_SHAPES_PER_FRAME] = {0};
int32_t shape_counter = 0;

static inline void reset_shape_memory_arena()
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

static inline void register_shape_to_draw(ShapeType type, void *data, int32_t size)
{
    if (!data || size <= 0)
        return;

    Shape *current_shape = &shapes[shape_counter];

    if (!allocate_shape_memory_arena(&current_shape->shape_frame_data, size))
        return;

    current_shape->type = type;

    // Copy shape data to memory arena block
    for (int32_t data_iterator = 0; data_iterator < size; ++data_iterator)
        ((byte *)current_shape->shape_frame_data)[data_iterator] = ((byte *)data)[data_iterator];

    ++shape_counter;
}


enum SHAPE_TYPE
{
    GS_LINE,
    GS_END_SHAPE_TYPE // <- Usado para el tamaño del arreglo de funciones puntero
};

/* 
    Las estructuras de los datos geometricos de las figuras sirven como contrato para definir como
    sera su modelo de memoria en la arena.
*/
typedef struct __tag_shape_line_contract__
{
    vec2f A, B;
    uint32_t color;
} ShapeLine;

#define SHAPE_STRUCT_LINE_SIZE sizeof(ShapeLine)

static inline void render_line_shape(const ShapeLine line_data)
{
    register_shape_to_draw( GS_LINE, (void*)&line_data, SHAPE_STRUCT_LINE_SIZE );
}

#endif // LIB_SHAPES_RENDER_MANAGEMENT_H