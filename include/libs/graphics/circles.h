
#ifndef LIB_GRAPHICS_SHAPES_CIRCLES_H
#define LIB_GRAPHICS_SHAPES_CIRCLES_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>
#include <limits.h>

#include <libs/render/render.h>
#include <libs/math/vectorial.h>

/*
 * Geometría de una fila del círculo.
 * Los límites X son relativos al centro.
 */
typedef struct
{
    int x_start;
    int x_end;
} CircleRow;

/*
 * Entrada de caché por radio.
 */
typedef struct CircleCache
{
    int radius;
    CircleRow *rows;
    struct CircleCache *next;
} CircleCache;

/*
 * Lista enlazada de geometrías almacenadas.
 */
static CircleCache *circle_cache_head = NULL;

/*
 * Busca una geometría existente por radio.
 */
static CircleCache *circle_cache_find(int radius)
{
    CircleCache *entry = circle_cache_head;

    while (entry != NULL)
    {
        if (entry->radius == radius)
            return entry;

        entry = entry->next;
    }

    return NULL;
}

/*
 * Calcula la geometría del círculo una sola vez
 * por cada creación de geometría.
 *
 * Cada fila contiene los extremos horizontales
 * que delimitan el círculo.
 */
static CircleRow *circle_geometry_create(int radius)
{
    if (radius < 0 || radius > 16383)
        return NULL;

    const size_t row_count = (size_t)radius * 2u + 1u;

    if (row_count > SIZE_MAX / sizeof(CircleRow))
        return NULL;

    CircleRow *rows = malloc(row_count * sizeof(CircleRow));

    if (rows == NULL)
        return NULL;

    const int radius_squared = radius * radius;

    for (int y = -radius; y <= radius; ++y)
    {
        const int remaining = radius_squared - y * y;
        int dx = radius;

        while (dx > 0 && dx * dx > remaining)
            --dx;

        rows[y + radius].x_start = -dx;
        rows[y + radius].x_end = dx;
    }

    return rows;
}

/*
 * Obtiene la geometría de un radio.
 *
 * cache = true:
 *   1. Busca la geometría existente.
 *   2. Si no existe, la calcula y la almacena.
 *
 * cache = false:
 *   1. Busca la geometría existente.
 *   2. Si existe, la reutiliza.
 *   3. Si no existe, calcula una geometría temporal.
 *
 * is_temporary indica si el llamador debe liberar
 * la memoria devuelta.
 */
static CircleRow *circle_geometry_get(
    int radius,
    bool cache,
    bool *is_temporary
)
{
    if (is_temporary != NULL)
        *is_temporary = false;

    if (radius < 0 || radius > 16383)
        return NULL;

    /*
     * Consultar siempre la caché, incluso cuando
     * el llamador no quiera guardar nuevas entradas.
     */
    CircleCache *entry = circle_cache_find(radius);

    if (entry != NULL)
        return entry->rows;

    /*
     * No existe una entrada para este radio.
     */
    CircleRow *rows = circle_geometry_create(radius);

    if (rows == NULL)
        return NULL;

    /*
     * No guardar la geometría si cache == false.
     */
    if (!cache)
    {
        if (is_temporary != NULL)
            *is_temporary = true;

        return rows;
    }

    /*
     * Guardar la geometría recién calculada.
     */
    entry = malloc(sizeof(CircleCache));

    if (entry == NULL)
    {
        free(rows);
        return NULL;
    }

    entry->radius = radius;
    entry->rows = rows;
    entry->next = circle_cache_head;

    circle_cache_head = entry;

    return entry->rows;
}

/*
 * Dibuja un círculo relleno en el framebuffer.
 *
 * center: posición del centro.
 * radius: radio en píxeles.
 * color: color de 32 bits.
 * cache: indica si se debe almacenar una geometría
 *        nueva cuando todavía no existe en la caché.
 */
static inline void draw_fill_circle(
    vec2f *center,
    int radius,
    uint32_t color,
    bool cache
)
{
    if (framebuffer == NULL || center == NULL)
        return;

    if (radius < 0 || radius > 16383)
        return;

    if (screen_width <= 0 || screen_height <= 0)
        return;

    /*
     * Obtener geometría cacheada o temporal.
     */
    bool temporary_geometry = false;

    CircleRow *rows = circle_geometry_get(
        radius,
        cache,
        &temporary_geometry
    );

    if (rows == NULL)
        return;

    const int cx = (int)center->x;
    const int cy = (int)center->y;

    /*
     * Limitar las filas al área visible.
     */
    int y_start = cy - radius;
    int y_end = cy + radius;

    if (y_start < 0)
        y_start = 0;

    if (y_end >= screen_height)
        y_end = screen_height - 1;

    /*
     * Dibujar el círculo mediante segmentos horizontales.
     */
    for (int y = y_start; y <= y_end; ++y)
    {
        const int row_index = y - cy + radius;

        if (row_index < 0 || row_index > radius * 2)
            continue;

        const CircleRow *geometry = &rows[row_index];

        int x_start = cx + geometry->x_start;
        int x_end = cx + geometry->x_end;

        /*
         * Recortar horizontalmente contra el framebuffer.
         */
        if (x_start < 0)
            x_start = 0;

        if (x_end >= screen_width)
            x_end = screen_width - 1;

        if (x_start > x_end)
            continue;

        uint32_t *framebuffer_row =
            framebuffer + (size_t)y * (size_t)screen_pitch;

        /*
         * Escribir únicamente los píxeles del segmento.
         */
        for (int x = x_start; x <= x_end; ++x)
            framebuffer_row[x] = color;
    }

    /*
     * Liberar únicamente la geometría temporal.
     * Las entradas de la caché permanecen disponibles.
     */
    if (temporary_geometry)
        free(rows);
}

/*
 * Libera todas las entradas de la caché.
 *
 * Debe llamarse al terminar de utilizar los círculos,
 * antes de finalizar el programa.
 */
static inline void clear_circle_cache(void)
{
    CircleCache *entry = circle_cache_head;

    while (entry != NULL)
    {
        CircleCache *next = entry->next;

        free(entry->rows);
        free(entry);

        entry = next;
    }

    circle_cache_head = NULL;
}

#endif /* LIB_GRAPHICS_SHAPES_CIRCLES_H */
