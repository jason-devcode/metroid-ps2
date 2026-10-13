#ifndef LIB_MATH_VECTORIAL_CALC_H
#define LIB_MATH_VECTORIAL_CALC_H

/**
 * Use cases>
 * 
 * For point coordinates:
 * 
 * float point[2] = { v.x, v.y };
 * 
 * For texture coordinates:
 * 
 * float texel[2] = { v.u * tex->w, v.v * tex->h };
 * 
 * For general pair float use cases:
 * 
 * float pair[2] = { v.a, v.b };
 * 
 */
typedef struct __tag_math_vec2f_structure__ {
    union { 
        float a[2]; 
        float a, b; 
        float x, y; 
        float u, v; 
    };
} vec2f;

#endif // LIB_MATH_VECTORIAL_CALC_H