/**
 * @file game.c
 * @brief Renders tiles directly from a native CI8 (T8) spritesheet texture in VRAM on PlayStation 2.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <malloc.h>

#include <libs/render/render.h>
#include <libs/texture/texture.h>
#include <libs/math/vectorial.h>
#include <libs/performance/fps.h>
#include <res/spritesheet_ci8.h>
#include <libs/events/event.h>
#include <libs/events/gamepad.h>

/* ================================================================= *
 * Constants & Configuration                                         *
 * ================================================================= */
#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 448
#define TOTAL_TILES   (SUB_SPRITE_COLS * SUB_SPRITE_ROWS)

#define SAMUS_IDLE      0
#define SAMUS_RUN_RIGHT 1

#define MOVEMENT_SPEED  350.0f
#define FRICTION_COEFF  0.05f
#define SPRITE_SCALE    2.0f

/* ================================================================= *
 * Player State & Action Callbacks                                   *
 * ================================================================= */
static vec2f player_position = {{350.0f, 165.0f}};
static vec2f player_velocity = {{0.0f, 0.0f}};

static void player_move_horizontal(void *data) {
    GamepadContext *ctx = (GamepadContext *)data;
    
    // Distancia respecto al centro (128)
    float h_axis_ratio = ((int)ctx->ljoy_h - ANALOG_STICK_CENTER) / 128.0f;
    
    player_velocity.x = h_axis_ratio * MOVEMENT_SPEED;
}

static GamepadActionMachine metroid_gamepad_machine = {
    .left_stick_moved = player_move_horizontal
};

/* ================================================================= *
 * Initialization Helpers                                            *
 * ================================================================= */
static bool init_game_systems(void) {
    if (!init_render(SCREEN_WIDTH, SCREEN_HEIGHT)) {
        close_render();
        return false;
    }

    gamepad.init(NULL);
    gamepad.add_actions_machine(&metroid_gamepad_machine);
    
    return true;
}

/* ================================================================= *
 * Main Application Entry Point                                      *
 * ================================================================= */
int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    // 1. Initialize rendering and input subsystems
    if (!init_game_systems()) {
        return -1;
    }

    // 2. Load CI8 texture directly into VRAM (1 byte per pixel)
    GSTEXTURE spritesheet_texture;
    if (!create_ci8_texture(&spritesheet_texture, SUB_SPRITESHEET_WIDTH, SUB_SPRITESHEET_HEIGHT, spritesheet_indices, spritesheet_palette)) {
        printf("Error al crear la textura CI8 nativa en VRAM\n");
        close_render();
        return -1;
    }

    // 3. Animation and render state variables
    float sprite_animation = 0.0f;
    float animation_speed  = 0.25f;
    int   sprite_col       = 0;
    bool  last_flip_x      = false;

    // 4. Main Game Loop
    while (game_running) {
        update_deltatime();
        gamepad.update(NULL);

        // Physics & Movement update
        player_position.x += player_velocity.x * deltatime;
        player_position.y += player_velocity.y * deltatime;
        player_velocity.x *= FRICTION_COEFF;

        // Update orientation flag based on horizontal velocity
        if (player_velocity.x != 0.0f) {
            last_flip_x = (player_velocity.x < 0.0f);
        }

        // Start a new frame
        begin_frame();

        // Select animation row and column
        int animation_row = ((int)player_velocity.x != 0) ? SAMUS_RUN_RIGHT : SAMUS_IDLE;
        int animation_col = (animation_row == SAMUS_RUN_RIGHT) ? sprite_col : SAMUS_IDLE;

        // Calculate UV coordinates on the spritesheet
        float u1 = (float)(animation_col * SUB_TILE_WIDTH);
        float v1 = (float)(animation_row * SUB_TILE_HEIGHT);
        float u2 = (float)((animation_col + 1) * SUB_TILE_WIDTH);
        float v2 = (float)((animation_row + 1) * SUB_TILE_HEIGHT);

        // Render player quad using native T8 texture (0xFF808080 balances gsKit brightness)
        render_quad(
            (int)player_position.x, (int)player_position.y,
            (float)SUB_TILE_WIDTH * SPRITE_SCALE, (float)SUB_TILE_HEIGHT * SPRITE_SCALE,
            u1, v1, u2, v2,
            last_flip_x, false,
            0xFF808080, &spritesheet_texture
        );

        // Advance sprite animation frame
        sprite_animation = (sprite_animation < (float)(SUB_SPRITE_COLS - 1)) 
                           ? sprite_animation + animation_speed 
                           : 0.0f;
        sprite_col = (int)floor(sprite_animation);

        // Submit draw batch to GPU
        draw();
    }

    close_render();
    return 0;
}