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

#define WIDTH 640
#define HEIGHT 448
#define TOTAL_TILES (SUB_SPRITE_COLS * SUB_SPRITE_ROWS)

int main(int argc, char *argv[])
{
    /* 1. Inicializar el motor de renderizado a resolución NTSC 640x448. */
    if (!init_render(WIDTH, HEIGHT))
    {
        close_render();
        return -1;
    }

    /* Cargar la textura CI8 nativa directamente en VRAM (1 byte por píxel) */
    GSTEXTURE spritesheet_texture;
    if (!create_ci8_texture(&spritesheet_texture, SUB_SPRITESHEET_WIDTH, SUB_SPRITESHEET_HEIGHT, spritesheet_indices, spritesheet_palette))
    {
        printf("Error al crear la textura CI8 nativa en VRAM\n");
        close_render();
        return -1;
    }

    /* 3. Bucle principal de juego y renderizado. */
    while (game_running)
    {
        update_deltatime();

        /* Iniciar un nuevo fotograma. */
        begin_frame();

        /* Dibujar los tiles en una cuadrícula centrada en la pantalla. */
        // float start_x = (WIDTH - (SUB_SPRITE_COLS * (SUB_TILE_WIDTH + 4))) / 2.0f;
        float start_x = 0.0f;
        float start_y = 0.0f;

        float scale = 1.0;

        for (int row = 0; row < SUB_SPRITE_ROWS; row++)
        {
            for (int col = 0; col < SUB_SPRITE_COLS; col++)
            {
                float render_x = start_x + (col * (SUB_TILE_WIDTH));
                float render_y = start_y + (row * (SUB_TILE_HEIGHT));

                /* Coordenadas UV en píxeles sobre el spritesheet completo */
                float u1 = (float)(col * SUB_TILE_WIDTH);
                float v1 = (float)(row * SUB_TILE_HEIGHT);
                float u2 = (float)((col + 1) * SUB_TILE_WIDTH);
                float v2 = (float)((row + 1) * SUB_TILE_HEIGHT);

                /* Renderizar el quad usando la textura T8 nativa 
                 * (Usamos 0x80808080 para equilibrar el brillo/intensidad de color en gsKit). */
                render_quad(render_x * scale, render_y * scale, 
                            (float)SUB_TILE_WIDTH * scale, (float)SUB_TILE_HEIGHT * scale, 
                            u1, v1, u2, v2, 
                            0xFF808080, &spritesheet_texture);
            }
        }

        /* Enviar el lote completo a la GPU. */
        draw();
    }

    close_render();
    return 0;
}