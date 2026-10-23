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

void create_square_texture(Texture *rect_texture)
{

    const int rect_w = 32;
    const int rect_h = 32;
    uint32_t rect_data_raw_image[rect_w * rect_h];

    for (int y = 0; y < rect_h; y++)
    {
        for (int x = 0; x < rect_w; x++)
        {
            int index = y * rect_w + x;

            // Si está en el borde exterior, asigna blanco; si no, transparente/vacío
            if (x == 0 || x == rect_w - 1 || y == 0 || y == rect_h - 1)
            {
                rect_data_raw_image[index] = 0xFFFFFFFF; // Borde (blanco visible)
            }
            else
            {
                rect_data_raw_image[index] = 0x00000000; // Centro vacío (transparente)
            }
        }
    }

    create_texture(rect_texture, rect_w, rect_h, rect_data_raw_image);
}

/**
 *
 */

#include <libs/camera/camera.h>

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
        3.5f, // Con esto ajustamos el zoom de la camara
        (vec2f){{0.0f, 0.0f}},
        (vec2f){{(float)SCREEN_WIDTH, (float)SCREEN_HEIGHT}},
        (CameraWorldBounds){
            .left_top_corner = (vec2f){{WORLD_LEFT_CORNER_X, WORLD_LEFT_CORNER_Y}},
            .right_bottom_corner = (vec2f){{WORLD_RIGHT_CORNER_X, WORLD_RIGHT_CORNER_Y}}},
        camera_chase_player);

    Texture rect_texture;
    create_square_texture(&rect_texture);

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

        render_quad(
            0, 0,
            camera.dimensions.w, camera.dimensions.h,
            0, 0,
            rect_texture.Width, rect_texture.Height,
            false, false,
            0xFFFFA000, &rect_texture);

        // Draw camera box
        render_quad(
            0, 0,
            camera.half_dimensions.w, camera.half_dimensions.h,
            0, 0,
            rect_texture.Width, rect_texture.Height,
            false, false,
            0xFF300030, &rect_texture);

        render_quad(
            (int)player_camera_coords.x, (int)player_camera_coords.y,
            display_sprite_w * camera.zoom, display_sprite_h * camera.zoom,
            player.current_frame.u1, player.current_frame.v1,
            player.current_frame.u2, player.current_frame.v2,
            false, false,
            0xFF808080, &samus.spritesheet.texture);

        // Draw player hitbox
        render_quad(
            (int)player_camera_coords.x, (int)player_camera_coords.y,
            display_sprite_w * camera.zoom, display_sprite_h * camera.zoom,
            0, 0,
            rect_texture.Width, rect_texture.Height,
            false, false,
            0xFF808080, &rect_texture);

        draw();
    }

    close_render();
    return 0;
}