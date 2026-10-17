#include <gsKit.h>
#include <dmaKit.h>
#include <stdlib.h>
#include <stdint.h>

/**
 * Carga de forma dinámica y genérica cualquier textura CI8 (8-bits indexada) 
 * y su paleta (CLUT) directamente en la VRAM de la PS2.
 * 
 * @param gsGlobal Puntero al contexto global de gsKit
 * @param width Ancho de la textura
 * @param height Alto de la textura
 * @param indices Puntero al array de bytes con los índices de los píxeles
 * @param palette Puntero al array de colores de 32 bits (ABGR) de la paleta
 * @return Puntero a la GSTEXTURE creada en VRAM, o NULL si falla.
 */
GSTEXTURE* create_ci8_texture_generic(GSGLOBAL *gsGlobal, int width, int height, const uint8_t *indices, const uint32_t *palette) {
    if (!gsGlobal || !indices || !palette) return NULL;

    // Reservar memoria para la estructura de la textura
    GSTEXTURE *texture = (GSTEXTURE *)malloc(sizeof(GSTEXTURE));
    if (!texture) return NULL;

    // Configurar metadatos para formato indexado de 8 bits (T8)
    texture->Width = width;
    texture->Height = height;
    texture->PSM = GS_PSM_T8;          // Formato indexado de 8 bits
    texture->ClutPSM = GS_PSM_CT32;    // Paleta en formato de 32 bits (ABGR)
    texture->Clut = (void *)palette;   // Asignar el puntero de la paleta directamente
    texture->Vram = NULL;              // gsKit gestionará la asignación de VRAM
    texture->MemWidth = gsKit_texture_mem_width(width, GS_PSM_T8);
    texture->BPP = 8;
    texture->Coord = GS_TEXTURE_COORDS_UV;

    // Enviar los índices y la CLUT a la VRAM del GS en una sola llamada
    int result = gsKit_texture_send(gsGlobal, texture, (void *)indices);
    if (result != GS_SUCCEEDED) {
        free(texture);
        return NULL;
    }

    return texture;
}