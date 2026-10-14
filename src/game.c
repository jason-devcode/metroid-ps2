#include <stdio.h>
#include <stdint.h>

#include <libs/render/render.h>
#include <libs/math/vectorial.h>
#include <libs/performance/fps.h>

// Graphics
#include <libs/graphics/circles.h>

#define WIDTH 640
#define HEIGHT 480

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