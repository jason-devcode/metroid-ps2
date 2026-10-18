/**
 * @file game.c
 * @brief Renders tiles directly from a native CI8 (T8) spritesheet texture in VRAM on PlayStation 2.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <malloc.h>
#include <libs/render/render.h>
#include <libs/math/vectorial.h>
#include <libs/performance/fps.h>
#include <res/spritesheet_ci8.h>

#include <libs/events/event.h>
#include <libs/events/gamepad.h>

#define WIDTH 640
#define HEIGHT 448
#define TOTAL_TILES (SUB_SPRITE_COLS * SUB_SPRITE_ROWS)

#define SAMUS_IDLE 0
#define SAMUS_RUN_RIGHT 1


vec2f player_position = {{ 0.0f, 165.0f }};
vec2f player_velocity = {{ 0.0f, 0.0f }};

void player_move_left( void* data ) {
    (void)data;
    player_velocity.x = -200.0;
}

void player_move_right( void* data ) {
    (void)data;
    player_velocity.x = 200.0;
}

GamepadActionMachine metroid_gamepad_machine = {
    .left_button_pressed  = player_move_left,
    .right_button_pressed = player_move_right
};

int main(int argc, char *argv[])
{
    /* 1. Inicializar el motor de renderizado a resolución NTSC 640x448. */
    if (!init_render(WIDTH, HEIGHT))
    {
        close_render();
        return -1;
    }

    gamepad.init( NULL );
    gamepad.add_actions_machine( &metroid_gamepad_machine );

    /* Cargar la textura CI8 nativa directamente en VRAM (1 byte por píxel) */
    GSTEXTURE spritesheet_texture;
    if (!create_ci8_texture(&spritesheet_texture, SUB_SPRITESHEET_WIDTH, SUB_SPRITESHEET_HEIGHT, spritesheet_indices, spritesheet_palette))
    {
        printf("Error al crear la textura CI8 nativa en VRAM\n");
        close_render();
        return -1;
    }

    int sprite_col = 0;

    float sprite_animation = 0.0f;
    float animation_speed = 0.25f;

    float scale = 1.5f;
    int sprite_row = 1;
    (void)sprite_row;

    float friction = 0.05;

    bool last_flip_x = false;

    /* 3. Bucle principal de juego y renderizado. */
    while (game_running)
    {
        update_deltatime();
        gamepad.update( NULL );

        player_position.x += player_velocity.x * deltatime;
        player_position.y += player_velocity.y * deltatime;
        player_velocity.x *= friction;

        last_flip_x = player_velocity.x != 0.0f ? player_velocity.x < 0.0f : last_flip_x;

        /* Iniciar un nuevo fotograma. */
        begin_frame();

        int animation_row = (int) player_velocity.x != 0 ? SAMUS_RUN_RIGHT : SAMUS_IDLE;
        int animation_col = animation_row == SAMUS_RUN_RIGHT ? sprite_col : SAMUS_IDLE;

        /* Coordenadas UV en píxeles sobre el spritesheet completo */
        float u1 = (float)(animation_col * SUB_TILE_WIDTH);
        float v1 = (float)(animation_row * SUB_TILE_HEIGHT);
        float u2 = (float)((animation_col + 1) * SUB_TILE_WIDTH);
        float v2 = (float)((animation_row + 1) * SUB_TILE_HEIGHT);
        

        /* Renderizar el quad usando la textura T8 nativa 
         * (Usamos 0x80808080 para equilibrar el brillo/intensidad de color en gsKit). */
        render_quad( (int) player_position.x , (int) player_position.y,
                    (float)SUB_TILE_WIDTH * scale, (float)SUB_TILE_HEIGHT * scale, 
                    u1, v1, u2, v2, last_flip_x, false,
                    0xFF808080, &spritesheet_texture);

        // Update sprite animation
        sprite_animation = sprite_animation < (float) (SUB_SPRITE_COLS - 1) ? sprite_animation + animation_speed : 0.0f;
        sprite_col = (int) floor( sprite_animation );

        /* Enviar el lote completo a la GPU. */
        draw();
    }

    close_render();
    return 0;
}
