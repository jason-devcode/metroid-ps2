/**
 * @file game.c
 * @brief Physics simulation of multiple bouncing particles using the batched quad rendering library on PlayStation 2.
 * @details Generates a procedural circle texture once and renders multiple particles with random velocities, sizes, and colors in a single batch.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <malloc.h>
#include <libs/render/render.h>
#include <libs/math/vectorial.h>
#include <libs/performance/fps.h>

#define CIRCLE_TEX_SIZE 240
#define WIDTH 640
#define HEIGHT 448
#define GRAVITY 980.0f
#define MAX_PARTICLES 100

/**
 * @brief Structure representing a bouncing particle in the physics simulation.
 */
typedef struct {
    vec2f pos;          /**< Position vector on screen. */
    vec2f vel;          /**< Velocity vector. */
    float radius;       /**< Particle radius. */
    uint32_t color;     /**< Tint color in ABGR format. */
} Particle;

/**
 * @brief Applies gravity acceleration to a velocity vector over time.
 * @param vel Pointer to the velocity vector.
 * @param dt Delta time elapsed since the last frame.
 */
void apply_gravity(vec2f *vel, float dt) {
    vel->y += GRAVITY * dt;
}

/**
 * @brief Updates a particle's physics, including gravity and wall/floor collision detection.
 * @param input Pointer to the particle to update.
 * @param dt Delta time elapsed since the last frame.
 */
void update_physics(Particle *input, float dt) {
    apply_gravity(&input->vel, dt);

    input->pos.x += input->vel.x * dt;
    input->pos.y += input->vel.y * dt;

    /* Floor collision with restitution. */
    if (input->pos.y > ((float)HEIGHT - input->radius)) {
        input->pos.y = (float)HEIGHT - input->radius;
        input->vel.y *= -0.85f;
        input->vel.x *= 0.75f;
    }

    /* Right wall collision. */
    if (input->pos.x > ((float)WIDTH - input->radius)) {
        input->pos.x = (float)WIDTH - input->radius;
        input->vel.x *= -0.75f;
    }

    /* Left wall collision. */
    if (input->pos.x < input->radius) {
        input->pos.x = input->radius;
        input->vel.x *= -0.75f;
    }
}

/**
 * @brief Main entry point of the application.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Exit status code.
 */
int main(int argc, char *argv[])
{
    /* 1. Initialize the rendering engine to NTSC 640x448 resolution. */
    if (!init_render(WIDTH, HEIGHT))
    {
        close_render();
        return -1;
    }

    /* 2. Generate the procedural circle texture, ensuring a clean alpha channel. */
    uint32_t *circle_pixels = (uint32_t *)memalign(64, CIRCLE_TEX_SIZE * CIRCLE_TEX_SIZE * sizeof(uint32_t));
    if (!circle_pixels)
    {
        close_render();
        return -1;
    }

    float center = (float)CIRCLE_TEX_SIZE / 2.0f;
    float radius_tex = center - 2.0f;

    for (int y = 0; y < CIRCLE_TEX_SIZE; y++)
    {
        for (int x = 0; x < CIRCLE_TEX_SIZE; x++)
        {
            float dx = (float)x + 0.5f - center;
            float dy = (float)y + 0.5f - center;
            float dist = sqrtf(dx * dx + dy * dy);

            /* Gradual alpha transition at the border. */
            float alpha = radius_tex + 0.5f - dist;

            if (alpha < 0.0f)
                alpha = 0.0f;

            if (alpha > 1.0f)
                alpha = 1.0f;

            uint8_t a = (uint8_t)(alpha * 128.0f + 0.5f);

            int idx = y * CIRCLE_TEX_SIZE + x;

            circle_pixels[idx] =
                ((uint32_t)a << 24) | 0x00FFFFFF;
        }
    }

    /* 3. Create and upload the texture to the PS2 VRAM. */
    GSTEXTURE circle_texture;
    if (!create_texture(&circle_texture, CIRCLE_TEX_SIZE, CIRCLE_TEX_SIZE, circle_pixels))
    {
        free(circle_pixels);
        close_render();
        return -1;
    }

    free(circle_pixels);

    /* Initialize the particle array with random sizes, velocities, and colors. */
    Particle particles[MAX_PARTICLES];
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particles[i].radius = 8.0f + ((float)rand() / (float)RAND_MAX) * 20.0f; /* Radii between 8 and 28 */
        
        particles[i].pos.x = particles[i].radius + ((float)rand() / (float)RAND_MAX) * (WIDTH - 2.0f * particles[i].radius);
        particles[i].pos.y = particles[i].radius + ((float)rand() / (float)RAND_MAX) * (HEIGHT / 2.0f);

        particles[i].vel.x = -200.0f + ((float)rand() / (float)RAND_MAX) * 400.0f; /* Random X velocity */
        particles[i].vel.y = -100.0f + ((float)rand() / (float)RAND_MAX) * 100.0f; /* Random Y velocity */

        uint8_t r = (uint8_t)(50 + rand() % 206);
        uint8_t g = (uint8_t)(50 + rand() % 206);
        uint8_t b = (uint8_t)(50 + rand() % 206);
        particles[i].color = RGB(r, g, b);
    }

    /* 4. Main game/render loop. */
    while (game_running)
    {
        update_deltatime();
        float dt = deltatime;

        /* Start a new frame by clearing the quad batch. */
        begin_frame();

        /* Update and render all particles in the same batch. */
        for (int i = 0; i < MAX_PARTICLES; i++) {
            update_physics(&particles[i], dt);

            float quad_size = particles[i].radius * 2.0f;
            float render_x = particles[i].pos.x - particles[i].radius;
            float render_y = particles[i].pos.y - particles[i].radius;
            render_quad(render_x, render_y, quad_size, quad_size, particles[i].color, &circle_texture);
        }

        /* Render the entire batch in a single draw call and synchronize. */
        draw();
    }

    close_render();
    return 0;
}