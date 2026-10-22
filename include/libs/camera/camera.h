#ifndef LIB_CAMERA_H
#define LIB_CAMERA_H

#include <libs/math/vectorial.h>

typedef void (*CameraChaseTargetCallback)(vec2f *position_to_update);

typedef struct __tag_camera_rect_world_bounds_structure__
{
    vec2f left_top_corner;
    vec2f right_bottom_corner;
} CameraWorldBounds;

typedef struct __tag_camera_2d_structure__
{
    vec2f position;
    vec2f dimensions;
    vec2f half_dimensions;
    CameraWorldBounds bounds;
    CameraChaseTargetCallback chase_callback;
} Camera;

/**
 * camera -> referencia a una instancia de la estructura de camara.
 * position -> posicion inicial por defecto de la camara.
 * dimensions -> ancho y alto de la camara en pixeles;
 * world_bounds -> estructura que contiene la informacion que define un area rectangular en la cual la camara se puede mover libremente.
 * chase_callback -> funcion puntero a una funcion la cual define la logica de como se debe de actualizar la posicion de la camara.
 */
static inline void create_camera(
    Camera *camera,
    vec2f position,
    vec2f dimensions,
    CameraWorldBounds world_bounds,
    CameraChaseTargetCallback chase_callback)
{
    if(!camera) return;

    camera->position = position;
    camera->dimensions = dimensions;
    camera->bounds = world_bounds;
    camera->chase_callback = chase_callback;
    
    vec2f_scale( &dimensions, 0.5f, &camera->half_dimensions );
}

/**
 * Esta funcion registra un callback el cual se llama en cada actualizacion de la camara para actualizar
 * su posicion en relacion a la actualizacion de posicion implementada en el callback.
 *
 * Por ejemplo un caso de uso bastante comun sera cuando se necesite hacer que la camara siga a un jugador
 * entonces creamos un callback que reciba el puntero de la posicion de la camara y actualizamos sus componentes
 * con las de la posicion del jugador.
 */
static inline void register_camera_target_callback(Camera *camera, CameraChaseTargetCallback chase_callback)
{
    if (!camera)
        return;
    camera->chase_callback = chase_callback;
}

/**
 * Esta funcion se encarga de actualizar la posicion de la camara de acuerdo a la logica
 * implementada en el callback de persecucion.
 *
 * Nota: Esta funcion solo comprueba si la instancia de la camara pasada como referencia
 * tiene establecido el puntero a la funcion con la logica de persecucion, por lo que no comprueba
 * si el puntero de la camara es un puntero valido ya que esta comprobacion se hace en la funcion
 * de actulaizacion de camara update_camera por simplificacion, por lo que no es recomendable llamarla
 * por fuera de ese ambito.
 */
static inline void update_camera_position(Camera *camera)
{
    if (!camera->chase_callback)
        return;
    camera->chase_callback(&camera->position);
}

/**
 *
 */
static inline void check_camera_bounds(Camera *camera)
{
}

/**
 * Esta funcion se llama en cada fotograma preferiblemente luego de habe actualizado la fisica y colisiones de las entidades.
 *
 * Esta funcion se encarga de actualizar la camara llamando primero al metodo de actualizacion por posicion de la camara
 * el cual debe llamar al callback de chase para actualizar la posicion de la camara de acuerdo a la logica de persecucion
 * establecida si es que el usuario configuro dicha logica, si no entonces se coloca la camara en la posicion por defecto
 * configurada.
 */
static inline void update_camera(Camera *camera)
{
    if (!camera)
        return;

    // 1. Actualizamos la posicion de la camara
    update_camera_position(camera);

    // 2. Comprobamos que la cámara este dentro de los límites establecidos.
    check_camera_bounds(camera);

    return;
}

#endif // LIB_CAMERA_H