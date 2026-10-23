#ifndef LIB_CAMERA_H
#define LIB_CAMERA_H

/**
 * @file camera.h
 * @brief 2D Camera management system with zoom, world bounds, and target chasing logic.
 */

#include <libs/math/vectorial.h>

/**
 * @brief Callback function pointer type for custom camera chasing logic.
 * 
 * @param position_to_update Pointer to the camera position vector to be modified.
 */
typedef void (*CameraChaseTargetCallback)(vec2f *position_to_update);

/**
 * @brief Represents a rectangular world boundary area for camera movement restriction.
 */
typedef struct __tag_camera_rect_world_bounds_structure__
{
    vec2f left_top_corner;     /**< Top-left corner coordinates of the world bounds */
    vec2f right_bottom_corner; /**< Bottom-right corner coordinates of the world bounds */
} CameraWorldBounds;

/**
 * @brief Represents a 2D camera with viewport dimensions, scaling, and boundary constraints.
 */
typedef struct __tag_camera_2d_structure__
{
    float zoom;                       /**< World zoom scale factor */
    vec2f position;                   /**< Current center position of the camera in world space */
    vec2f dimensions;                 /**< Width and height of the camera viewport in pixels */
    vec2f half_dimensions;            /**< Half of the viewport dimensions */
    CameraWorldBounds bounds;         /**< Rectangular movement constraints within the world */
    CameraChaseTargetCallback chase_callback; /**< Optional target chasing callback function */
} Camera;

/**
 * @brief Initializes a camera instance with specified parameters.
 * 
 * @param camera Pointer to the camera instance to initialize.
 * @param zoom Initial zoom scale factor.
 * @param position Initial center position in world space.
 * @param dimensions Viewport width and height in pixels.
 * @param world_bounds Rectangular area boundaries restricting camera movement.
 * @param chase_callback Optional function pointer defining target chasing behavior.
 */
static inline void create_camera(
    Camera *camera,
    float zoom,
    vec2f position,
    vec2f dimensions,
    CameraWorldBounds world_bounds,
    CameraChaseTargetCallback chase_callback)
{
    if (!camera)
        return;

    camera->zoom = zoom;
    camera->position = position;
    camera->dimensions = dimensions;
    camera->bounds = world_bounds;
    camera->chase_callback = chase_callback;

    vec2f_scale(&dimensions, 0.5f, &camera->half_dimensions);
}

/**
 * @brief Registers or updates the target chasing callback function for the camera.
 * 
 * @param camera Pointer to the camera instance.
 * @param chase_callback Function pointer defining how the camera position updates.
 */
static inline void register_camera_target_callback(Camera *camera, CameraChaseTargetCallback chase_callback)
{
    if (!camera)
        return;
    camera->chase_callback = chase_callback;
}

/**
 * @brief Updates the camera position using the registered chase callback logic.
 * 
 * @note This internal function assumes the camera pointer is valid and delegates 
 *       null checks to the parent update routine.
 * 
 * @param camera Pointer to the camera instance.
 */
static inline void update_camera_position(Camera *camera)
{
    if (!camera->chase_callback)
        return;
    camera->chase_callback(&camera->position);
}

/**
 * @brief Clamps the camera position within the configured world boundaries, accounting for zoom.
 * 
 * @param camera Pointer to the camera instance.
 */
static inline void check_camera_bounds(Camera *camera)
{
    vec2f *position = &camera->position;
    vec2f *half_dimensions = &camera->half_dimensions;
    CameraWorldBounds *bounds = &camera->bounds;

    float zoom = camera->zoom;
    float rev_zoom = 1.0f / zoom;

    // Check left world bound
    if (position->x - half_dimensions->w * rev_zoom < bounds->left_top_corner.x)
        position->x = bounds->left_top_corner.x + half_dimensions->w * rev_zoom;

    // Check top world bound
    if (position->y - half_dimensions->h * rev_zoom < bounds->left_top_corner.y)
        position->y = bounds->left_top_corner.y + half_dimensions->h * rev_zoom;

    // Check right world bound
    if (position->x + half_dimensions->w * rev_zoom > bounds->right_bottom_corner.x)
        position->x = bounds->right_bottom_corner.x - half_dimensions->w * rev_zoom;

    // Check bottom world bound
    if (position->y + half_dimensions->h * rev_zoom > bounds->right_bottom_corner.y)
        position->y = bounds->right_bottom_corner.y - half_dimensions->h * rev_zoom;
}

/**
 * @brief Updates the camera state each frame, executing target tracking and boundary clamping.
 * 
 * @note Should be called every frame, preferably after entity physics and collision updates.
 * 
 * @param camera Pointer to the camera instance.
 */
static inline void update_camera(Camera *camera)
{
    if (!camera)
        return;

    // 1. Update camera position via target chasing logic
    update_camera_position(camera);

    // 2. Ensure camera remains within established world bounds
    check_camera_bounds(camera);
}

/**
 * @brief Transforms world coordinates into camera-relative viewport coordinates.
 * 
 * @param camera Pointer to the camera instance.
 * @param world_pos Pointer to the input position vector in world space.
 * @param output Pointer to the output vector to store screen/viewport coordinates.
 */
static inline void world_coords_to_camera_coords(Camera *camera, vec2f *world_pos, vec2f *output)
{
    if (!camera || !world_pos || !output)
        return;
    output->x = camera->half_dimensions.w + (world_pos->x - camera->position.x) * camera->zoom;
    output->y = camera->half_dimensions.h + (world_pos->y - camera->position.y) * camera->zoom;
}

#endif // LIB_CAMERA_H