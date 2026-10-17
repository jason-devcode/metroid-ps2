/**
 * @file game.c
 * @brief Renders tiles directly from a single CI8 spritesheet using UV coordinates on PlayStation 2.
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

    /* 2. Decodificar el spritesheet CI8 usando la paleta optimizada en ABGR. */
    uint32_t *sheet_pixels = (uint32_t *)memalign(64, SUB_SPRITESHEET_WIDTH * SUB_SPRITESHEET_HEIGHT * sizeof(uint32_t));
    if (!sheet_pixels)
    {
        close_render();
        return -1;
    }

    for (int i = 0; i < SUB_SPRITESHEET_WIDTH * SUB_SPRITESHEET_HEIGHT; i++)
    {
        uint8_t color_index = spritesheet_indices[i];
        // La paleta ya viene empaquetada correctamente desde Python en formato compatible para la PS2
        sheet_pixels[i] = spritesheet_palette[color_index];
    }

    /* 3. Subir el spritesheet completo a una sola textura en la VRAM. */
    GSTEXTURE spritesheet_texture;
    if (!create_texture(&spritesheet_texture, SUB_SPRITESHEET_WIDTH, SUB_SPRITESHEET_HEIGHT, sheet_pixels))
    {
        printf("Error al crear la textura del spritesheet completo\n");
        free(sheet_pixels);
        close_render();
        return -1;
    }

    // Liberar el buffer temporal de la CPU
    free(sheet_pixels);

    /* 4. Bucle principal de juego y renderizado. */
    while (game_running)
    {
        update_deltatime();

        /* Iniciar un nuevo fotograma. */
        begin_frame();

        /* Dibujar los tiles en una cuadrícula centrada en la pantalla. */
        float start_x = (WIDTH - (SUB_SPRITE_COLS * (SUB_TILE_WIDTH + 4))) / 2.0f;
        float start_y = 80.0f;

        for (int row = 0; row < SUB_SPRITE_ROWS; row++)
        {
            for (int col = 0; col < SUB_SPRITE_COLS; col++)
            {
                float render_x = start_x + (col * (SUB_TILE_WIDTH + 8));
                float render_y = start_y + (row * (SUB_TILE_HEIGHT + 8));

                /* Coordenadas UV en píxeles sobre el spritesheet completo */
                float u1 = (float)(col * SUB_TILE_WIDTH);
                float v1 = (float)(row * SUB_TILE_HEIGHT);
                float u2 = (float)((col + 1) * SUB_TILE_WIDTH);
                float v2 = (float)((row + 1) * SUB_TILE_HEIGHT);

                // Renderizar el quad con la textura única y sus UVs
                render_quad(render_x, render_y, 
                            (float)SUB_TILE_WIDTH, (float)SUB_TILE_HEIGHT, 
                            u1, v1, u2, v2, 
                            0x80808080, &spritesheet_texture);
            }
        }

        /* Enviar el lote completo a la GPU. */
        draw();
    }

    close_render();
    return 0;
}