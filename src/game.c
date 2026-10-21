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

void create_square_texture( Texture* rect_texture ) {
    
    const int rect_w = 32;
    const int rect_h = 32;
    uint32_t rect_data_raw_image[ rect_w * rect_h ];

    for (int y = 0; y < rect_h; y++) {
        for (int x = 0; x < rect_w; x++) {
            int index = y * rect_w + x;
            
            // Si está en el borde exterior, asigna blanco; si no, transparente/vacío
            if (x == 0 || x == rect_w-1 || y == 0 || y == rect_h-1) {
                rect_data_raw_image[index] = 0xFFFFFFFF; // Borde (blanco visible)
            } else {
                rect_data_raw_image[index] = 0x00000000; // Centro vacío (transparente)
            }
        }
    }

    create_texture( rect_texture, rect_w, rect_h, rect_data_raw_image );
}

/**
 * 
 */


typedef struct __struct_tag_camera__ {
    int32_t x1, y1;
    int32_t w, h;
    int32_t half_w, half_h;
} Camera;

int main( int argc, char** argv )
{
    (void)argc;
    (void)argv;

    if (!init_game_systems() || !init_player())
    {
        close_render();
        return -1;
    }

    Camera camera = {
        0, 0,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        SCREEN_WIDTH >> 1, SCREEN_HEIGHT >> 1
    };

    Texture rect_texture;
    create_square_texture( &rect_texture );

    // Main Game Loop
    while (game_running)
    {
        update_deltatime();
        gamepad.update(NULL);
        update_player();

        ///// UPDATE CAMERA LOGIC
        camera.x1 = (int) player.position.x;
        camera.y1 = (int) player.position.y;

        //check camera bounds

        // check left bound
        if( camera.x1 - camera.half_w < WORLD_LEFT_CORNER_X )
            camera.x1 = WORLD_LEFT_CORNER_X + camera.half_w;
        
        // check right bound
        if( camera.x1 + camera.half_w > WORLD_RIGHT_CORNER_X )
            camera.x1 = WORLD_RIGHT_CORNER_X - camera.half_w;
        
        
        // check top bound
        if( camera.y1 - camera.half_h < WORLD_LEFT_CORNER_Y )
            camera.y1 = WORLD_LEFT_CORNER_Y + camera.half_h;
        
        // check bottom bound
        if( camera.y1 + camera.half_h > WORLD_RIGHT_CORNER_Y )
            camera.y1 = WORLD_RIGHT_CORNER_Y - camera.half_h;

        /////////////////////////////////////

        begin_frame();

        int player_camera_pos_x = SCREEN_WIDTH / 2 + (int)player.position.x - camera.x1;
        int player_camera_pos_y = SCREEN_HEIGHT / 2 + (int)player.position.y - camera.y1;

        // Draw camera box
        render_quad(
            0, 0,
            camera.w, camera.h,
            0, 0,
            rect_texture.Width, rect_texture.Height,
            false, false,
            0xFF300030, &rect_texture);

        render_quad(
            (int)player_camera_pos_x, (int)player_camera_pos_y,
            display_sprite_w, display_sprite_h,
            player.current_frame.u1, player.current_frame.v1,
            player.current_frame.u2, player.current_frame.v2,
            false, false,
            0xFF808080, &samus.spritesheet.texture);

        
        // Draw player hitbox
        render_quad(
            (int)player_camera_pos_x, (int)player_camera_pos_y,
            display_sprite_w, display_sprite_h,
            0, 0,
            rect_texture.Width, rect_texture.Height,
            false, false,
            0xFF808080, &rect_texture);

        draw();
    }

    close_render();
    return 0;
}