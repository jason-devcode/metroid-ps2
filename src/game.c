#include <stdio.h>
#include <stdint.h>

#include <libs/render/render.h>
#include <libs/math/vectorial.h>
#include <libs/performance/fps.h>

#define WIDTH 640
#define HEIGHT 480

void draw_fill_circle(vec2f *center, int radius, uint32_t color)
{
    int y_start = (int)center->y - radius;
    int y_end = (int)center->y + radius + 1;
    int x_start = (int)center->x - radius;
    int x_end = (int)center->x + radius + 1;

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
            if ((dx * dx + dy2) <= (radius * radius))
            {
                row[x] = color;
            }
        }
    }
}

typedef struct {
    vec2f pos;
    vec2f vel;
    int radius;
    uint32_t color;
} Particle;

#define GRAVITY 980.0f

void apply_gravity(vec2f *vel) {
    vel->y += GRAVITY * deltatime;
}

void update_physics(Particle *input) {
    apply_gravity(&input->vel);

    input->pos.x += input->vel.x * deltatime;
    input->pos.y += input->vel.y * deltatime;

    if( input->pos.y > (HEIGHT - input->radius) ) {
        input->pos.y = HEIGHT - input->radius;
        input->vel.y *= -0.85;
        input->vel.x *= 0.75;

    }
    
    if( input->pos.x > (WIDTH - input->radius) ) {
        input->pos.x = WIDTH - input->radius;
        input->vel.x *= -0.75;
    }

    if( input->pos.x < input->radius ) {
        input->pos.x = input->radius;
        input->vel.x *= -0.75;
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

    Particle m = {
        .pos = {{ WIDTH / 2.0f, HEIGHT / 2.0f }},
        .vel = {{ 950.0f, 0.0f }},
        .color = RGB(255, 0 ,0),
        .radius = 20
    };


    while (game_running)
    {
        update_deltatime();

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                break;
        }

        clear_screen();

        update_physics(&m);

        draw_fill_circle(&m.pos, m.radius, m.color);

        draw();
    }

    close_render();

    return 0;
}