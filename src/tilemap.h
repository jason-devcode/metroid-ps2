#ifndef TEST_TILEMAP_LOGIC_H
#define TEST_TILEMAP_LOGIC_H

#define TILEMAP_SIZE_X 16
#define TILEMAP_SIZE_Y 16

#include <libs/shapes/shapes.h>
#include <libs/camera/camera.h>
#include <libs/world/world.h>

int tilemap[TILEMAP_SIZE_X][TILEMAP_SIZE_Y] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0}};

void draw_tilemap(Camera *camera)
{
    vec2f tile_screen_pos = (vec2f){{0.0f, 0.0f}};

    for (float Y = camera->position.y - camera->half_dimensions.h / camera->zoom; Y < camera->position.y + camera->half_dimensions.h / camera->zoom + HEIGHT_PER_WORLD_TILE; Y += HEIGHT_PER_WORLD_TILE)
    {

        for (float X = camera->position.x - camera->half_dimensions.w / camera->zoom; X < camera->position.x + camera->half_dimensions.w / camera->zoom + WIDTH_PER_WORLD_TILE; X += WIDTH_PER_WORLD_TILE)
        {
            vec2f tile_world_pos = (vec2f){{
                floorf(X / WIDTH_PER_WORLD_TILE) * WIDTH_PER_WORLD_TILE,
                floorf(Y / HEIGHT_PER_WORLD_TILE) * HEIGHT_PER_WORLD_TILE,
            }};

            world_coords_to_camera_coords(camera, &tile_world_pos, &tile_screen_pos);

            render_rectangle_shape(
                (ShapeRectangle){
                    .left_top_corner = tile_screen_pos,
                    .right_bottom_corner = (vec2f){{tile_screen_pos.x + WIDTH_PER_WORLD_TILE * camera->zoom, tile_screen_pos.y + HEIGHT_PER_WORLD_TILE * camera->zoom}},
                    .color = 0xFF005050});
        }
    }
}

#endif // TEST_TILEMAP_LOGIC_H