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

#define SPRITE_SCALE 2.0f

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

int main( int argc, char** argv )
{
    (void)argc;
    (void)argv;

    if (!init_game_systems() || !init_player())
    {
        close_render();
        return -1;
    }

    // Main Game Loop
    while (game_running)
    {
        update_deltatime();
        gamepad.update(NULL);
        update_player();
        begin_frame();

        render_quad(
            (int)player.position.x, (int)player.position.y,
            display_sprite_w, display_sprite_h,
            player.current_frame.u1, player.current_frame.v1,
            player.current_frame.u2, player.current_frame.v2,
            false, false,
            0xFF808080, &samus.spritesheet.texture);

        draw();
    }

    close_render();
    return 0;
}