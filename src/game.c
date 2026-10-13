#include <SDL.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    // Inicializar SDL con soporte de vídeo
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Error al inicializar SDL: %s\n", SDL_GetError());
        return -1;
    }

    // Configurar el modo de vídeo (resolución estándar para PS2: 640x448, 16 bits por píxel)
    SDL_Surface *screen = SDL_SetVideoMode(640, 448, 16, SDL_SWSURFACE);
    if (!screen) {
        printf("Error al establecer el modo de vídeo: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    printf("¡SDL1 inicializado correctamente en PS2!\n");

    int running = 1;
    SDL_Event event;

    // Parámetros del círculo
    int center_x = 320;
    int center_y = 224;
    int radius = 80;

    // Bucle principal del juego / prueba
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
        }

        // 1. Limpiar pantalla con un color de fondo (Azul oscuro / Metroid)
        Uint32 bg_color = SDL_MapRGB(screen->format, 15, 25, 45);
        SDL_FillRect(screen, NULL, bg_color);

        // 2. Manipulación directa de píxeles para dibujar un círculo (Color Rojo brillante)
        Uint32 circle_color = SDL_MapRGB(screen->format, 255, 60, 60);

        int pitch = screen->pitch / 2; // Paso en formato de 16 bits (Uint16)
        Uint16 *pixels = (Uint16 *)screen->pixels;

        // Recorremos una caja delimitadora alrededor del círculo para optimizar
        int y_start = center_y - radius;
        int y_end = center_y + radius;
        int x_start = center_x - radius;
        int x_end = center_x + radius;

        // Limites de la pantalla
        if (y_start < 0) y_start = 0;
        if (y_end > 448) y_end = 448;
        if (x_start < 0) x_start = 0;
        if (x_end > 640) x_end = 640;

        for (int y = y_start; y < y_end; y++) {
            int dy = y - center_y;
            int dy2 = dy * dy;
            Uint16 *row = pixels + (y * pitch);

            for (int x = x_start; x < x_end; x++) {
                int dx = x - center_x;
                // Ecuación del círculo: dx*dx + dy*dy <= r*r
                if ((dx * dx + dy2) <= (radius * radius)) {
                    row[x] = (Uint16)circle_color;
                }
            }
        }

        // Actualizar pantalla (doble búfer / flip)
        SDL_Flip(screen);
    }

    SDL_Quit();
    return 0;
}