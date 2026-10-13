#ifndef LIB_MATH_VECTORIAL_CALC_H
#define LIB_MATH_VECTORIAL_CALC_H

/**
 * @file lib_math_vectorial_calc.h
 * @brief Inline 2D vector mathematical operations optimized for the PlayStation 2 (Emotion Engine).
 * @details Provides a versatile `vec2f` structure with multiple access aliases and a set of
 *          high-performance, branchless inline functions designed to avoid stack copies
 *          and pipeline stalls on MIPS architectures.
 */

#include <math.h>

/**
 * @brief A versatile 2D vector / pair of floats with multiple access aliases.
 *
 * This structure uses an anonymous union to allow accessing the same 2D data
 * through different semantic naming conventions (spatial coordinates, texture coordinates,
 * generic pairs, or raw arrays).
 *
 * @par Use cases:
 *
 * - **For point coordinates:**
 *   ```c
 *   float point[2] = { v.x, v.y };
 *   ```
 *
 * - **For texture coordinates:**
 *   ```c
 *   float texel[2] = { v.u * tex->w, v.v * tex->h };
 *   ```
 *
 * - **For general pair float use cases:**
 *   ```c
 *   float pair[2] = { v.a, v.b };
 *   ```
 */
typedef union
{
    float raw[2]; /**< Acceso por índice [0, 1]. */
    struct
    {
        float x, y; /**< Coordenadas espaciales. */
    };
    struct
    {
        float u, v; /**< Coordenadas de textura. */
    };
    struct
    {
        float a, b; /**< Componentes de par genérico. */
    };
} vec2f;

/**
 * @brief Subtracts two 2D vectors component-wise (result = input0 - input1).
 *
 * @param input0 Pointer to the minuend vector (read-only).
 * @param input1 Pointer to the subtrahend vector (read-only).
 * @param result Pointer to the structure where the result will be written.
 */
static inline void vec2f_sub(const vec2f *input0, const vec2f *input1, vec2f *result)
{
    result->a = input0->a - input1->a;
    result->b = input0->b - input1->b;
}

/**
 * @brief Multiplies two 2D vectors component-wise (result = input0 * input1).
 *
 * Useful for scaling texture coordinates or modulating colors.
 *
 * @param input0 Pointer to the first vector (read-only).
 * @param input1 Pointer to the second vector (read-only).
 * @param result Pointer to the structure where the result will be written.
 */
static inline void vec2f_mul(const vec2f *input0, const vec2f *input1, vec2f *result)
{
    result->a = input0->a * input1->a;
    result->b = input0->b * input1->b;
}

/**
 * @brief Multiplies a 2D vector by a scalar value (result = input * scalar).
 *
 * @param input Pointer to the input vector (read-only).
 * @param scalar The floating-point scalar value.
 * @param result Pointer to the structure where the result will be written.
 */
static inline void vec2f_scale(const vec2f *input, float scalar, vec2f *result)
{
    result->a = input->a * scalar;
    result->b = input->b * scalar;
}

/**
 * @brief Calculates the dot product of two 2D vectors.
 *
 * @param input0 Pointer to the first vector (read-only).
 * @param input1 Pointer to the second vector (read-only).
 * @return The scalar dot product value (`float`).
 */
static inline float vec2f_dot(const vec2f *input0, const vec2f *input1)
{
    return (input0->a * input1->a) + (input0->b * input1->b);
}

/**
 * @brief Calculates the squared magnitude (length) of a 2D vector.
 *
 * Useful for distance comparisons to avoid expensive square root operations.
 *
 * @param input Pointer to the input vector (read-only).
 * @return The squared length (`float`).
 */
static inline float vec2f_length_sq(const vec2f *input)
{
    return (input->a * input->a) + (input->b * input->b);
}

/**
 * @brief Calculates the magnitude (length) of a 2D vector.
 *
 * Note: Uses `sqrtf`. On PS2, keep in mind that hardware square root
 * or reciprocal square root instructions can be leveraged if performance is critical.
 *
 * @param input Pointer to the input vector (read-only).
 * @return The length of the vector (`float`).
 */
static inline float vec2f_length(const vec2f *input)
{
    return __builtin_sqrtf((input->a * input->a) + (input->b * input->b));
}
/**
 * @brief Normalizes a 2D vector branchlessly.
 *
 * Uses conditional moves (`movz`/`movn` on MIPS) instead of control-flow branches
 * to prevent pipeline stalls on the PS2 Emotion Engine.
 *
 * @param input Pointer to the input vector (read-only).
 * @param result Pointer to the structure where the normalized vector will be written.
 */
static inline void vec2f_normalize(const vec2f *input, vec2f *result)
{
    float len_sq = (input->a * input->a) + (input->b * input->b);

    // If len_sq > 0, compute 1/sqrt(len_sq); otherwise, safely assign 0.0f without branches.
    float inv_len = (len_sq > 0.0f) ? (1.0f / __builtin_sqrtf(len_sq)) : 0.0f;

    result->a = input->a * inv_len;
    result->b = input->b * inv_len;
}
/**
 * @brief Linearly interpolates between two 2D vectors (LERP).
 *
 * @param input0 Pointer to the starting vector (t = 0.0).
 * @param input1 Pointer to the ending vector (t = 1.0).
 * @param t Interpolation factor (usually between 0.0 and 1.0).
 * @param result Pointer to the structure where the result will be written.
 */
static inline void vec2f_lerp(const vec2f *input0, const vec2f *input1, float t, vec2f *result)
{
    result->a = input0->a + (input1->a - input0->a) * t;
    result->b = input0->b + (input1->b - input0->b) * t;
}

#endif // LIB_MATH_VECTORIAL_CALC_H