#ifndef LIB_GRAPHICS_SHAPES_CIRCLES_H
#define LIB_GRAPHICS_SHAPES_CIRCLES_H

#include <stdint.h>

// For framebuffer accesss
#include <libs/render/render.h>
#include <libs/math/vectorial.h>


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

#endif // LIB_GRAPHICS_SHAPES_CIRCLES_H