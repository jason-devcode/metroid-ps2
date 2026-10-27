#ifndef LIB_SHAPES_DRAW_IMPLEMENTATIONS_H
#define LIB_SHAPES_DRAW_IMPLEMENTATIONS_H

/**
 * @file shapes.h
 * @brief Geometric shape drawing implementations and render pipeline for PS2 using gsKit.
 * 
 * This module defines shape data contracts, primitive draw call functions, 
 * type enumerations, and the dispatch system to render registered shapes per frame.
 */

#include <gsKit.h>
#include <stdint.h>
#include <libs/math/vectorial.h>
#include <libs/shapes/shape_memory_arena.h>

/**
 * @brief Packs a standard color into a gsKit-compatible format.
 * 
 * @param color The raw 32-bit color value.
 * @return uint64_t The GS-compatible packed color.
 */
static inline uint64_t pack_gs_color(uint32_t color);

/**
 * @brief Supported shape type enumeration identifiers.
 */
enum SHAPE_TYPE
{
    GS_LINE,                       /**< Simple line primitive type. */
    GS_CIRCLE,                     /**< Circle primitive type using line segments. */
    GS_RECTANGLE,
    GS_TRIANGLE_WIRE,              /**< Wireframe triangle primitive type. */
    GS_SHAPE_DRAW_FUNCTIONS_COUNT  /**< Total count of shape types used for array sizing. */
};

/** @brief Function pointer signature for drawing a registered shape. */
typedef void (*ShapeDrawFunction)(GSGLOBAL* gs_global, void* shape_data);


/** @brief Function pointer table mapping shape types to their respective draw implementations. */
static ShapeDrawFunction shape_functions_draw_calls[GS_SHAPE_DRAW_FUNCTIONS_COUNT];

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

/**
 * @brief Geometric contract structure defining a rectangle shape's memory layout.
 */
typedef struct __tag_shape_rectangle_contract__
{
    vec2f left_top_corner;     /**< Starting 2D point of the rectangle. */
    vec2f right_bottom_corner; /**< Ending 2D point of the rectangle. */
    uint32_t color;            /**< Packed color of the line. */
} ShapeRectangle;

/** @brief Macro evaluating the exact memory footprint of a ShapeRectangle structure. */
#define SHAPE_STRUCT_RECTANGLE_SIZE sizeof(ShapeRectangle)

/**
 * @brief Draw call implementation for rendering a rectangle primitive via gsKit.
 * 
 * @param gs_global Pointer to the gsKit global context.
 * @param retangle_data Pointer to the ShapeRectangle structure containing coordinates and color.
 */
static void shape_draw_call_rectangle(GSGLOBAL* gs_global, ShapeRectangle* retangle_data)
{
    gsKit_prim_line(
        gs_global,
        retangle_data->left_top_corner.x, retangle_data->left_top_corner.y,
        retangle_data->right_bottom_corner.x, retangle_data->left_top_corner.y, 0,
        pack_gs_color(retangle_data->color)
    );
    gsKit_prim_line(
        gs_global,
        retangle_data->right_bottom_corner.x, retangle_data->left_top_corner.y,
        retangle_data->right_bottom_corner.x, retangle_data->right_bottom_corner.y, 0,
        pack_gs_color(retangle_data->color)
    );
    gsKit_prim_line(
        gs_global,
        retangle_data->right_bottom_corner.x, retangle_data->right_bottom_corner.y,
        retangle_data->left_top_corner.x, retangle_data->right_bottom_corner.y, 0,
        pack_gs_color(retangle_data->color)
    );
    gsKit_prim_line(
        gs_global,
        retangle_data->left_top_corner.x, retangle_data->right_bottom_corner.y,
        retangle_data->left_top_corner.x, retangle_data->left_top_corner.y, 0,
        pack_gs_color(retangle_data->color)
    );
}

/**
 * @brief Geometric contract structure defining a triangle shape's memory layout.
 */
typedef struct __tag_shape_triangle_contract__
{
    vec2f A;         /**< First vertex of the triangle. */
    vec2f B;         /**< Second vertex of the triangle. */
    vec2f C;         /**< Third vertex of the triangle. */
    uint32_t color;  /**< Packed color of the triangle. */
} ShapeTriangle;

/** @brief Macro evaluating the exact memory footprint of a ShapeTriangle structure. */
#define SHAPE_STRUCT_TRIANGLE_SIZE sizeof(ShapeTriangle)

/**
 * @brief Draw call implementation for rendering a wireframe triangle via gsKit lines.
 * 
 * @param gs_global Pointer to the gsKit global context.
 * @param triangle_data Pointer to the ShapeTriangle structure containing vertices and color.
 */
static void shape_draw_call_triangle_wire(GSGLOBAL* gs_global, ShapeTriangle* triangle_data)
{
    uint64_t packed_color = pack_gs_color(triangle_data->color);

    // Draw the three sides of the triangle
    gsKit_prim_line(gs_global, triangle_data->A.x, triangle_data->A.y, triangle_data->B.x, triangle_data->B.y, 0, packed_color);
    gsKit_prim_line(gs_global, triangle_data->B.x, triangle_data->B.y, triangle_data->C.x, triangle_data->C.y, 0, packed_color);
    gsKit_prim_line(gs_global, triangle_data->C.x, triangle_data->C.y, triangle_data->A.x, triangle_data->A.y, 0, packed_color);
}

/**
 * @brief Geometric contract structure defining a circle shape's memory layout.
 */
typedef struct __tag_shape_circle_contract__
{
    vec2f center;    /**< Center 2D point of the circle. */
    float radius;    /**< Radius of the circle. */
    uint32_t color;  /**< Packed color of the circle. */
} ShapeCircle;

/** @brief Macro evaluating the exact memory footprint of a ShapeCircle structure. */
#define SHAPE_STRUCT_CIRCLE_SIZE sizeof(ShapeCircle)

/**
 * @brief Draw call implementation for rendering a circle via line segments.
 * 
 * @param gs_global Pointer to the gsKit global context.
 * @param circle_data Pointer to the ShapeCircle structure containing center, radius, and color.
 */
static void shape_draw_call_circle(GSGLOBAL* gs_global, ShapeCircle* circle_data)
{
    const int num_segments = 24; // Balance entre suavidad visual y rendimiento en PS2
    const float theta = 2.0f * M_PI / (float)num_segments;
    const float c = cosf(theta);
    const float s = sinf(theta);

    float x = circle_data->radius;
    float y = 0.0f;

    uint64_t packed_color = pack_gs_color(circle_data->color);
    float xc = circle_data->center.x;
    float yc = circle_data->center.y;

    for (int i = 0; i < num_segments; i++)
    {
        float next_x = c * x - s * y;
        float next_y = s * x + c * y;

        gsKit_prim_line(
            gs_global,
            xc + x, yc + y,
            xc + next_x, yc + next_y,
            0,
            packed_color
        );

        x = next_x;
        y = next_y;
    }
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
 * @brief Submits a circle shape to the render queue for the current frame.
 * 
 * @param circle_data The ShapeCircle structure containing center, radius, and color.
 */
static inline void render_circle_shape(const ShapeCircle circle_data)
{
    register_shape_to_draw(GS_CIRCLE, (void*)&circle_data, SHAPE_STRUCT_CIRCLE_SIZE);
}

/**
 * @brief Submits a wireframe triangle shape to the render queue for the current frame.
 * 
 * @param triangle_data The ShapeTriangle structure containing vertices and color.
 */
static inline void render_triangle_wire_shape(const ShapeTriangle triangle_data)
{
    register_shape_to_draw(GS_TRIANGLE_WIRE, (void*)&triangle_data, SHAPE_STRUCT_TRIANGLE_SIZE);
}

/**
 * @brief Submits a rectangle shape to the render queue for the current frame.
 * 
 * @param rectangle_data The ShapeRectangle structure containing vertices and color.
 */
static inline void render_rectangle_shape(const ShapeRectangle rectangle_data)
{
    register_shape_to_draw(GS_RECTANGLE, (void*)&rectangle_data, SHAPE_STRUCT_RECTANGLE_SIZE);
}

static inline void render_rectangle(float x1, float y1, float x2, float y2, uint32_t color)
{
    render_rectangle_shape((ShapeRectangle){
        .left_top_corner     = (vec2f){{ x1, y1 }},
        .right_bottom_corner = (vec2f){{ x2, y2 }},
        .color = color
    });
}

/**
 * @brief Initializes the shape rendering system and binds draw call handlers.
 */
static inline void initialize_draw_shapes_system(void)
{
    shape_functions_draw_calls[GS_LINE]          = (ShapeDrawFunction)shape_draw_call_simple_line;
    shape_functions_draw_calls[GS_CIRCLE]        = (ShapeDrawFunction)shape_draw_call_circle;
    shape_functions_draw_calls[GS_RECTANGLE]     = (ShapeDrawFunction)shape_draw_call_rectangle;
    shape_functions_draw_calls[GS_TRIANGLE_WIRE] = (ShapeDrawFunction)shape_draw_call_triangle_wire;
}


#endif // LIB_SHAPES_DRAW_IMPLEMENTATIONS_H