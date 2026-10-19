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
#include <libs/texture/texture.h>
#include <libs/math/vectorial.h>
#include <libs/performance/fps.h>
#include <res/spritesheet_ci8.h>
#include <libs/events/event.h>
#include <libs/events/gamepad.h>
#include <libs/sprite/sprite.h>
#include <libs/sprite/animation.h>

/* ================================================================= *
 * Constants & Configuration                                         *
 * ================================================================= */
#define SCREEN_WIDTH      640
#define SCREEN_HEIGHT     448
#define TOTAL_TILES       (SUB_SPRITE_COLS * SUB_SPRITE_ROWS)

#define MOVEMENT_SPEED    350.0f
#define FRICTION_COEFF    0.1f
#define SPRITE_SCALE      2.0f
#define ANALOG_CENTER_DIV 128.0f

/* ================================================================= *
 * Types & Structures                                                *
 * ================================================================= */
typedef struct {
    vec2f position;
    vec2f velocity;
    bool facing_left;
} Player;

typedef struct {
    Sprite spritesheet;
    SpriteAnimation idle_left;
    SpriteAnimation idle_right;
    SpriteAnimation walk_left;
    SpriteAnimation walk_right;
} PlayerAnimations;

/* ================================================================= *
 * Global State                                                      *
 * ================================================================= */
static Player player = {
    .position = {{350.0f, 165.0f}},
    .velocity = {{0.0f, 0.0f}},
    .facing_left = false
};

static PlayerAnimations samus;

/* ================================================================= *
 * Player Input & Movement                                           *
 * ================================================================= */
static void player_move_horizontal(void *data)
{
    const GamepadContext *ctx = (GamepadContext *)data;
    float h_axis_ratio = ((int)ctx->ljoy_h - ANALOG_STICK_CENTER) / ANALOG_CENTER_DIV;

    player.velocity.x = h_axis_ratio * MOVEMENT_SPEED;
    
    if (player.velocity.x != 0.0f) {
        player.facing_left = (player.velocity.x < 0.0f);
    }
}

static GamepadActionMachine metroid_gamepad_machine = {
    .left_stick_moved = player_move_horizontal
};

/* ================================================================= *
 * Initialization Helpers                                            *
 * ================================================================= */
static bool init_game_systems(void)
{
    if (!init_render(SCREEN_WIDTH, SCREEN_HEIGHT)) {
        close_render();
        return false;
    }

    gamepad.init(NULL);
    gamepad.add_actions_machine(&metroid_gamepad_machine);
    return true;
}

static bool init_player_assets(void)
{
    if (!create_ci8_sprite(
            &samus.spritesheet,
            SUB_SPRITESHEET_WIDTH,
            SUB_SPRITESHEET_HEIGHT,
            SUB_SPRITE_COLS,
            SUB_SPRITE_ROWS,
            SUB_TILE_WIDTH,
            SUB_TILE_HEIGHT,
            spritesheet_indices,
            spritesheet_palette))
    {
        printf("Error al crear la textura CI8 nativa en VRAM\n");
        return false;
    }

    create_sprite_animation(&samus.idle_left,  &samus.spritesheet, 0.0f,  0.0f, 0.0f, 1.0f,                    0.0f, true,  false, false);
    create_sprite_animation(&samus.idle_right, &samus.spritesheet, 0.0f,  0.0f, 0.0f, 1.0f,                    0.0f, false, false, false);
    create_sprite_animation(&samus.walk_right, &samus.spritesheet, 0.25f, 0.0f, 0.0f, (float)SUB_SPRITE_COLS - 1, 1.0f, false, false, true);
    create_sprite_animation(&samus.walk_left,  &samus.spritesheet, 0.25f, 0.0f, 0.0f, (float)SUB_SPRITE_COLS - 1, 1.0f, true,  false, true);

    return true;
}

static void update_physics(void)
{
    player.position.x += player.velocity.x * deltatime;
    player.position.y += player.velocity.y * deltatime;
    player.velocity.x *= FRICTION_COEFF;
}

static void update_animation(SpriteAnimation **current_animation, SpriteFrame *out_frame)
{
    const bool is_moving = fabsf(player.velocity.x) >= 1.0f;

    *current_animation = is_moving
        ? (player.facing_left ? &samus.walk_left : &samus.walk_right)
        : (player.facing_left ? &samus.idle_left : &samus.idle_right);

    update_sprite_animation(*current_animation, out_frame);
}

/* ================================================================= *
 * Main Application Entry Point                                      *
 * ================================================================= */
int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    if (!init_game_systems() || !init_player_assets()) {
        close_render();
        return -1;
    }

    const float display_sprite_w = (float)SUB_TILE_WIDTH * SPRITE_SCALE;
    const float display_sprite_h = (float)SUB_TILE_HEIGHT * SPRITE_SCALE;

    SpriteAnimation *current_animation = &samus.idle_right;
    SpriteFrame samus_current_frame;

    // Main Game Loop
    while (game_running)
    {
        update_deltatime();
        gamepad.update(NULL);
        update_physics();

        update_animation(&current_animation, &samus_current_frame);
            
        begin_frame();

        render_quad(
            (int)player.position.x, (int)player.position.y,
            display_sprite_w, display_sprite_h,
            samus_current_frame.u1, samus_current_frame.v1, 
            samus_current_frame.u2, samus_current_frame.v2,
            false, false,
            0xFF808080, &samus.spritesheet.texture
        );

        draw();
    }

    close_render();
    return 0;
}