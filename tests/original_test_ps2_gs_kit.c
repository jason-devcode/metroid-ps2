#include <gsKit.h>
#include <dmaKit.h>
#include <malloc.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#include <libs/performance/fps.h>

#define WIDTH 640
#define HEIGHT 448

typedef struct {
    float x;
    float y;
} vec2f;

typedef struct {
    vec2f pos;
    vec2f vel;
    int radius;
    u32 color;
} Particle;

#define GRAVITY 980.0f


void apply_gravity(vec2f *vel) {
    vel->y += GRAVITY * deltatime;
}

void update_physics(Particle *input) {
    apply_gravity(&input->vel);

    input->pos.x += input->vel.x * deltatime;
    input->pos.y += input->vel.y * deltatime;

    if (input->pos.y > (HEIGHT - input->radius)) {
        input->pos.y = HEIGHT - input->radius;
        input->vel.y *= -0.85f;
        input->vel.x *= 0.75f;
    }
    
    if (input->pos.x > (WIDTH - input->radius)) {
        input->pos.x = WIDTH - input->radius;
        input->vel.x *= -0.75f;
    }

    if (input->pos.x < input->radius) {
        input->pos.x = (float)input->radius;
        input->vel.x *= -0.75f;
    }
}

// Rasterizar círculo relleno optimizado
void draw_fill_circle(u32 *fb, int cx, int cy, int radius, u32 color) {
    int xmin = cx - radius;
    int xmax = cx + radius;
    int ymin = cy - radius;
    int ymax = cy + radius;

    if (xmin < 0) xmin = 0;
    if (xmax >= WIDTH) xmax = WIDTH - 1;
    if (ymin < 0) ymin = 0;
    if (ymax >= HEIGHT) ymax = HEIGHT - 1;

    int r2 = radius * radius;

    for (int y = ymin; y <= ymax; y++) {
        int dy = y - cy;
        int dy2 = dy * dy;
        u32 *row = &fb[y * WIDTH];
        for (int x = xmin; x <= xmax; x++) {
            int dx = x - cx;
            if (dx * dx + dy2 <= r2) {
                row[x] = color;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC, D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);

    GSGLOBAL *gs = gsKit_init_global();
    gs->Mode = GS_MODE_NTSC;
    gs->Width = WIDTH;
    gs->Height = HEIGHT;
    gs->PSM = GS_PSM_CT32;
    gs->DoubleBuffering = GS_SETTING_ON;

    gsKit_init_screen(gs);
    gsKit_mode_switch(gs, GS_PERSISTENT);

    u32 *framebuffer = (u32 *)memalign(64, WIDTH * HEIGHT * sizeof(u32));
    if (!framebuffer) return -1;

    // Crear una línea de fondo pre-renderizada para limpiar el buffer instantáneamente con memcpy
    u32 *clear_row = (u32 *)memalign(64, WIDTH * sizeof(u32));
    for (int x = 0; x < WIDTH; x++) {
        clear_row[x] = 0xFF191919; // Gris oscuro
    }

    GSTEXTURE tex;
    tex.Width = WIDTH;
    tex.Height = HEIGHT;
    tex.PSM = GS_PSM_CT32;
    tex.Mem = framebuffer;
    tex.Filter = GS_FILTER_NEAREST;
    tex.TBW = (WIDTH + 63) / 64;
    tex.Vram = gsKit_vram_alloc(gs, gsKit_texture_size(WIDTH, HEIGHT, GS_PSM_CT32), 0);

    Particle m = {
        .pos = { WIDTH / 2.0f, HEIGHT / 4.0f },
        .vel = { 400.0f, 0.0f },
        .color = 0xFF0000FF, 
        .radius = 25
    };

    while (1) {
        update_deltatime();
        gsKit_vsync_wait();

        // Limpieza ultrarrápida por filas usando memcpy (aprovecha el ancho de banda del EE sin lag)
        for (int y = 0; y < HEIGHT; y++) {
            memcpy(&framebuffer[y * WIDTH], clear_row, WIDTH * sizeof(u32));
        }

        // Actualizar físicas y dibujar círculo
        update_physics(&m);
        draw_fill_circle(framebuffer, (int)m.pos.x, (int)m.pos.y, m.radius, m.color);

        // Subir textura y renderizar quad por hardware
        gsKit_texture_upload(gs, &tex);
        dmaKit_wait_fast();

        gsKit_prim_sprite_texture(gs, &tex,
                                  0.0f, 0.0f,
                                  0.0f, 0.0f,
                                  (float)WIDTH, (float)HEIGHT,
                                  (float)WIDTH, (float)HEIGHT,
                                  0,
                                  GS_SETREG_RGBAQ(128, 128, 128, 0x80, 0x00));

        gsKit_queue_exec(gs);
        gsKit_sync_flip(gs);
    }

    free(clear_row);
    free(framebuffer);
    return 0;
}