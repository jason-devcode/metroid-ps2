#ifndef PLAYER_SAMUS_H
#define PLAYER_SAMUS_H

#include <libs/math/vectorial.h>
#include <libs/sprite/sprite.h>
#include <libs/sprite/animation.h>
#include <libs/events/gamepad.h>
#include <libs/texture/texture.h>
#include <res/spritesheet_ci8.h>
#include <libs/math/vectorial.h>
#include <libs/state_machine/fsm.h>

#define ANALOG_CENTER_DIV 128.0f

#define MOVEMENT_SPEED 350.0f
#define FRICTION_COEFF 0.125f

#define GRAVITY 1200

typedef struct
{
    vec2f position;
    vec2f velocity;

    SpriteAnimation *current_animation;
    SpriteFrame current_frame;
    FiniteStateMachine fsm;
} Player;

typedef struct
{
    Sprite spritesheet;
    SpriteAnimation idle_left;
    SpriteAnimation idle_right;
    SpriteAnimation walk_left;
    SpriteAnimation walk_right;

    SpriteAnimation fall_left;
    SpriteAnimation fall_right;
} PlayerAnimations;

/* ================================================================= *
 * Types & Structures                                                *
 * ================================================================= */

/* ================================================================= *
 * Global State                                                      *
 * ================================================================= */
static Player player = {
    .position = {{350.0f, 165.0f}},
    .velocity = {{0.0f, 0.0f}},
    .current_animation = NULL,
    .current_frame = {0, 0, 0, 0}};

static PlayerAnimations samus;

/* ================================================================= *
 * Player Input & Movement                                           *
 * ================================================================= */
static void player_move_horizontal(void *data)
{
    const GamepadContext *ctx = (GamepadContext *)data;
    float h_axis_ratio = ((int)ctx->ljoy_h - ANALOG_STICK_CENTER) / ANALOG_CENTER_DIV;
    float v_axis_ratio = ((int)ctx->ljoy_v - ANALOG_STICK_CENTER) / ANALOG_CENTER_DIV;

    if (fabs(h_axis_ratio) > 0.8f)
        player.velocity.x = h_axis_ratio * MOVEMENT_SPEED;

    if (player.velocity.y == 0.0f && v_axis_ratio < -0.8f)
        player.velocity.y = -550.0f;
}

static GamepadActionMachine metroid_gamepad_machine = {
    .left_stick_moved = player_move_horizontal};

static bool init_player_assets(void)
{
    gamepad.add_actions_machine(&metroid_gamepad_machine);

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

    create_sprite_animation(&samus.idle_right, &samus.spritesheet, 0.0225f, 0.0f, 0.0f, 2.0f, 0, false, false, true);
    create_sprite_animation(&samus.idle_left, &samus.spritesheet, 0.0225f, 0.0f, 0.0f, 2.0f, 1, true, false, true);

    create_sprite_animation(&samus.walk_right, &samus.spritesheet, 0.25f, 0.0f, 0.0f, 9.0f, 2, false, false, true);
    create_sprite_animation(&samus.walk_left, &samus.spritesheet, 0.25f, 0.0f, 0.0f, 9.0f, 3, true, false, true);

    create_sprite_animation(&samus.fall_right, &samus.spritesheet, 0.0f, 15.0f, 15.0f, 16.0f, 0, false, false, true);
    create_sprite_animation(&samus.fall_left, &samus.spritesheet, 0.0f, 15.0f, 15.0f, 16.0f, 1, true, false, true);

    return true;
}

enum SAMUS_STATES
{
    SAMUS_IDLE_RIGHT_STATE,
    SAMUS_IDLE_LEFT_STATE,
    SAMUS_WALK_LEFT,
    SAMUS_WALK_RIGHT,
    SAMUS_FALL_LEFT,
    SAMUS_FALL_RIGHT,
};

// ----- idle left state
static void enter_samus_idle_left_state(FSM *fsm)
{
    SpriteAnimation **current_animation = (SpriteAnimation **)fsm->context;
    *current_animation = &samus.idle_left;
}

static void step_samus_idle_left_state(FSM *fsm)
{
    if ((int)player.velocity.x != 0)
        fsm_change(fsm, player.velocity.x < 0.0f ? SAMUS_WALK_LEFT : SAMUS_WALK_RIGHT);

    if (player.velocity.y != 0.0f)
        fsm_change(fsm, SAMUS_FALL_LEFT);
}

// ----- idle right state
static void enter_samus_idle_right_state(FSM *fsm)
{
    SpriteAnimation **current_animation = (SpriteAnimation **)fsm->context;
    *current_animation = &samus.idle_right;
}

static void step_samus_idle_right_state(FSM *fsm)
{
    if ((int)player.velocity.x != 0)
        fsm_change(fsm, player.velocity.x < 0.0f ? SAMUS_WALK_LEFT : SAMUS_WALK_RIGHT);

    if (player.velocity.y != 0.0f)
        fsm_change(fsm, SAMUS_FALL_RIGHT);
}

// ----- walk left state
static void enter_samus_walk_left_state(FSM *fsm)
{
    SpriteAnimation **current_animation = (SpriteAnimation **)fsm->context;
    *current_animation = &samus.walk_left;
}

static void step_samus_walk_left_state(FSM *fsm)
{
    if (player.velocity.x > 0.0f)
    {
        fsm_change(fsm, SAMUS_WALK_RIGHT);
        return;
    }

    if ((int)player.velocity.x == 0)
        fsm_change(fsm, SAMUS_IDLE_LEFT_STATE);

    if (player.velocity.y != 0.0f)
        fsm_change(fsm, SAMUS_FALL_LEFT);
}

// ----- walk right state
static void enter_samus_walk_right_state(FSM *fsm)
{
    SpriteAnimation **current_animation = (SpriteAnimation **)fsm->context;
    *current_animation = &samus.walk_right;
}

static void step_samus_walk_right_state(FSM *fsm)
{
    if (player.velocity.x < 0.0f)
    {
        fsm_change(fsm, SAMUS_WALK_LEFT);
        return;
    }

    if ((int)player.velocity.x == 0)
        fsm_change(fsm, SAMUS_IDLE_RIGHT_STATE);

    if (player.velocity.y != 0.0f)
        fsm_change(fsm, SAMUS_FALL_RIGHT);
}

// ----- fall left state
static void enter_samus_fall_left_state(FSM *fsm)
{
    SpriteAnimation **current_animation = (SpriteAnimation **)fsm->context;
    *current_animation = &samus.fall_left;
}

static void step_samus_fall_left_state(FSM *fsm)
{
    if (player.velocity.x > 0.0f)
    {
        fsm_change(fsm, SAMUS_FALL_RIGHT);
        return;
    }

    if ((int)player.velocity.y == 0)
    {
        fsm_change(fsm, SAMUS_IDLE_LEFT_STATE);
        return;
    }
}

// ----- fall right state
static void enter_samus_fall_right_state(FSM *fsm)
{
    SpriteAnimation **current_animation = (SpriteAnimation **)fsm->context;
    *current_animation = &samus.fall_right;
}

static void step_samus_fall_right_state(FSM *fsm)
{
    if (player.velocity.x < 0.0f)
    {
        fsm_change(fsm, SAMUS_FALL_LEFT);
        return;
    }

    if ((int)player.velocity.y == 0)
    {
        fsm_change(fsm, SAMUS_IDLE_RIGHT_STATE);
        return;
    }
}

static bool init_player()
{
    if (!init_player_assets())
        return false;

    fsm_set_context(&player.fsm, (StateContext)&player.current_animation);

    fsm_register(
        &player.fsm,
        SAMUS_IDLE_LEFT_STATE,
        (StateFSM){
            .enter = enter_samus_idle_left_state,
            .step = step_samus_idle_left_state});

    fsm_register(
        &player.fsm,
        SAMUS_IDLE_RIGHT_STATE,
        (StateFSM){
            .enter = enter_samus_idle_right_state,
            .step = step_samus_idle_right_state});

    fsm_register(
        &player.fsm,
        SAMUS_WALK_LEFT,
        (StateFSM){
            .enter = enter_samus_walk_left_state,
            .step = step_samus_walk_left_state});

    fsm_register(
        &player.fsm,
        SAMUS_WALK_RIGHT,
        (StateFSM){
            .enter = enter_samus_walk_right_state,
            .step = step_samus_walk_right_state});

    fsm_register(
        &player.fsm,
        SAMUS_FALL_LEFT,
        (StateFSM){
            .enter = enter_samus_fall_left_state,
            .step = step_samus_fall_left_state});

    fsm_register(
        &player.fsm,
        SAMUS_FALL_RIGHT,
        (StateFSM){
            .enter = enter_samus_fall_right_state,
            .step = step_samus_fall_right_state});

    fsm_change(&player.fsm, SAMUS_IDLE_RIGHT_STATE);

    return true;
}

static void update_player_physics()
{
    player.position.x += player.velocity.x * deltatime;
    player.position.y += player.velocity.y * deltatime;

    player.velocity.x *= FRICTION_COEFF;
    player.velocity.y += GRAVITY * deltatime;

    float level_floor = 400.0f;

    if (player.position.y + SUB_TILE_HEIGHT * 2 >= level_floor)
    {
        player.position.y = level_floor - SUB_TILE_HEIGHT * 2;
        player.velocity.y = 0.0f;
    }
}

static void update_player()
{
    update_player_physics();
    fsm_step(&player.fsm);
    update_sprite_animation(player.current_animation, &player.current_frame);
}

#endif // PLAYER_SAMUS_H