
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include <libs/render/render.h>
#include <libs/math/vectorial.h>
#include <libs/performance/fps.h>
#include <libs/graphics/circles.h>

#define WIDTH 640
#define HEIGHT 480

#define PARTICLE_COUNT 100
#define PARTICLE_RADIUS 5

#define GRAVITY 500.0f
#define INITIAL_SPEED_MIN 100.0f
#define INITIAL_SPEED_MAX 350.0f

typedef struct
{
    vec2f pos;
    vec2f vel;
    int radius;
    uint32_t color;
} Particle;

static uint32_t random_color(void)
{
    uint8_t r = (uint8_t)(rand() % 256);
    uint8_t g = (uint8_t)(rand() % 256);
    uint8_t b = (uint8_t)(rand() % 256);

    return RGB(r, g, b);
}

static float random_range(float min, float max)
{
    return min + ((float)rand() / (float)RAND_MAX) * (max - min);
}

static void apply_gravity(vec2f *vel)
{
    vel->y += GRAVITY * deltatime;
}

static void update_physics(Particle *particle)
{
    apply_gravity(&particle->vel);

    particle->pos.x += particle->vel.x * deltatime;
    particle->pos.y += particle->vel.y * deltatime;

    /* Rebote contra el suelo. */
    if (particle->pos.y > HEIGHT - particle->radius - 1)
    {
        particle->pos.y = HEIGHT - particle->radius - 1;
        particle->vel.y *= -0.85f;
        particle->vel.x *= 0.98f;
    }

    /* Rebote contra el borde derecho. */
    if (particle->pos.x > WIDTH - particle->radius - 1)
    {
        particle->pos.x = WIDTH - particle->radius - 1;
        particle->vel.x *= -0.85f;
    }

    /* Rebote contra el borde izquierdo. */
    if (particle->pos.x < particle->radius)
    {
        particle->pos.x = particle->radius;
        particle->vel.x *= -0.85f;
    }

    /* Rebote contra el techo. */
    if (particle->pos.y < particle->radius)
    {
        particle->pos.y = particle->radius;
        particle->vel.y *= -0.85f;
    }
}

static void initialize_particles(Particle *particles)
{
    const float center_x = WIDTH / 2.0f;
    const float center_y = HEIGHT / 2.0f;

    for (int i = 0; i < PARTICLE_COUNT; ++i)
    {
        /*
         * Ángulo aleatorio en todas las direcciones.
         */
        const float angle = random_range(
            0.0f,
            6.28318530718f
        );

        const float speed = random_range(
            INITIAL_SPEED_MIN,
            INITIAL_SPEED_MAX
        );

        particles[i].pos.x = center_x;
        particles[i].pos.y = center_y;

        particles[i].vel.x = speed * cosf(angle);
        particles[i].vel.y = speed * sinf(angle);

        particles[i].radius = PARTICLE_RADIUS;
        particles[i].color = random_color();
    }
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    srand(12345);

    Particle particles[PARTICLE_COUNT];

    if (!init_render(WIDTH, HEIGHT))
    {
        clear_circle_cache();
        close_render();
        return -1;
    }

    initialize_particles(particles);

    while (game_running)
    {
        update_deltatime();

        clear_screen();

        for (int i = 0; i < PARTICLE_COUNT; ++i)
        {
            update_physics(&particles[i]);

            /*
             * Todas comparten el radio y la geometría cacheada.
             * El color y la posición son independientes.
             */
            draw_fill_circle(
                &particles[i].pos,
                particles[i].radius,
                particles[i].color,
                true
            );
        }

        draw();
    }

    clear_circle_cache();
    close_render();

    return 0;
}
