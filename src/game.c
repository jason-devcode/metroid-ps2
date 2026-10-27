/**
 * @file game.c
 * @brief Renders tiles directly from a native CI8 (T8) spritesheet texture in VRAM on PlayStation 2.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <malloc.h>
#include <stdbool.h>

#include <libs/render/render.h>
#include <libs/performance/fps.h>

#include <libs/camera/camera.h>

#include <libs/shapes/shapes.h>

#include "samus.h"

/* ================================================================= *
 * Constants & Configuration                                         *
 * ================================================================= */
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 448
#define TOTAL_TILES (SUB_SPRITE_COLS * SUB_SPRITE_ROWS)

#define SPRITE_SCALE 1.0f

const float display_sprite_w = (float)SUB_TILE_WIDTH * SPRITE_SCALE;
const float display_sprite_h = (float)SUB_TILE_HEIGHT * SPRITE_SCALE;

/* ================================================================= *
 * Initialization Helpers                                            *
 * ================================================================= */
static bool init_game_systems(void)
{
    if (!init_render(SCREEN_WIDTH, SCREEN_HEIGHT))
    {
        close_render();
        return false;
    }

    gamepad.init(NULL);
    return true;
}

void camera_chase_player(vec2f *camera_position)
{
    camera_position->x = player.position.x;
    camera_position->y = player.position.y;
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (!init_game_systems() || !init_player())
    {
        close_render();
        return -1;
    }


    // Esto es el codigo beta de la camara

    Camera camera;

    create_camera(
        &camera,
        1.0f, // Con esto ajustamos el zoom de la camara
        (vec2f){{0.0f, 0.0f}},
        // (vec2f){{(float)SCREEN_WIDTH / 8, (float)SCREEN_HEIGHT / 8}},
        // (vec2f){{(float)SCREEN_WIDTH / 1.25, (float)SCREEN_HEIGHT / 1.25}},
        (vec2f){{0.0f,0.0f}},
        (vec2f){{SCREEN_WIDTH, SCREEN_HEIGHT}},
        (CameraWorldBounds){
            .left_top_corner = (vec2f){{WORLD_LEFT_CORNER_X, WORLD_LEFT_CORNER_Y}},
            .right_bottom_corner = (vec2f){{WORLD_RIGHT_CORNER_X, WORLD_RIGHT_CORNER_Y}}},
        camera_chase_player);

    vec2f player_camera_coords = (vec2f){{0.0f, 0.0f}};

    // Main Game Loop
    while (game_running)
    {
        update_deltatime();
        gamepad.update(NULL);
        update_player();
        update_camera(&camera);

        world_coords_to_camera_coords(&camera, &player.position, &player_camera_coords);

        begin_frame();
        reset_shape_memory_arena();

        render_rectangle(
            camera.screen_position.x, camera.screen_position.y,
            camera.screen_position.x + camera.half_dimensions.w, camera.screen_position.y + camera.half_dimensions.h,
            0xFFFF0000);

        // Draw camera box
        render_rectangle(
            camera.screen_position.x, camera.screen_position.y,
            camera.screen_position.x + camera.dimensions.w - 1, camera.screen_position.y + camera.dimensions.h - 1,
            0xFF00FFFF);

        // Draw player hitbox
        render_rectangle(
            player_camera_coords.x, player_camera_coords.y,
            player_camera_coords.x + display_sprite_w * camera.zoom, player_camera_coords.y + display_sprite_h * camera.zoom,
            0xFF808080);

        render_circle_shape(
            (ShapeCircle){
                .center = {{SCREEN_WIDTH >> 1, SCREEN_HEIGHT >> 1}},
                .radius = 90.0f,
                .color = 0xFFFFFF00});

        render_triangle_wire_shape((ShapeTriangle){
            .A = {{ 100.0f, 100.0f }},
            .B = {{ 400.0f, 200.0f }},
            .C = {{ 200.0f, 400.0f }},
            .color = 0xFF00FFFF
        });

        render_quad(
            (int)player_camera_coords.x, (int)player_camera_coords.y,
            display_sprite_w * camera.zoom, display_sprite_h * camera.zoom,
            player.current_frame.u1, player.current_frame.v1,
            player.current_frame.u2, player.current_frame.v2,
            false, false,
            0xFF808080, &samus.spritesheet.texture);

        draw();
    }

    close_render();
    return 0;
}