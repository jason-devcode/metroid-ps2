#include <stdio.h>
#include <stdint.h>

#include <libs/render/render.h>
#include <libs/math/vectorial.h>

#define WIDTH 640
#define HEIGHT 480

void draw_fill_circle(vec2f *center, int radius, uint32_t color)
{
    // Recorremos una caja delimitadora alrededor del círculo para optimizar
    int y_start = (int)center->y - radius;
    int y_end = (int)center->y + radius;
    int x_start = (int)center->x - radius;
    int x_end = (int)center->x + radius;

    // Limites de la pantalla
    if (y_start < 0)
        y_start = 0;
    if (y_end > screen_height)
        y_end = screen_height;
    if (x_start < 0)
        x_start = 0;
    if (x_end > screen_width)
        x_end = screen_width;

    for (int y = y_start; y < y_end; y++)
    {
        int dy = y - center->y;
        int dy2 = dy * dy;
        uint32_t *row = framebuffer + (y * screen_pitch);

        for (int x = x_start; x < x_end; x++)
        {
            int dx = x - center->x;
            // Ecuación del círculo: dx*dx + dy*dy <= r*r
            if ((dx * dx + dy2) <= (radius * radius))
            {
                row[x] = color;
            }
        }
    }
}

int main(int argc, char *argv[])
{
    if (!init_render(WIDTH, HEIGHT))
    {
        close_render();
        return -1;
    }

    SDL_Event event;

    vec2f center = (vec2f){WIDTH / 2, HEIGHT / 2};

    // Bucle principal del juego / prueba
    while (game_running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                break;
        }

        clear_screen();

        draw_fill_circle( &center, 128, RGB(255, 0, 0) );

        draw();
    }

    close_render();

    return 0;
}