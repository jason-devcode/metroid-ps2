# Metroid Fan-Game

## Requisitos - Historias de usuario

Como jugador quiero controlar a un personaje basado en el personaje de Samus Aran utilizando el traje que usaba el SA-X en el juego de Metroid Fusión.

Quiero poder construir niveles basados en tilemaps.

Quiero que cada tile en el tilemap se identifique con un ID unico de 16 bits.

A traves de los IDs de cada tile podre acceder a una estructura de datos de tile, donde podre definir el aspecto del tile, sus colisiones y fisicas.

Los tiles tendrán tamaños fijos de 16x16 pixeles.

Quiero que mi personaje pueda colisionar con los tiles del mapa a traves de sus ejes X y Y.

## Fase 1 - Preparar el proyecto base

- [x] Preparar el entorno de desarrollo PS2SDK en un contenedor docker.
- [x] Crear un script para compilar el proyecto completo.
- [x] Agregar la licencia MIT.
- [x] Crear y subir el repositorio del proyecto.

## Fase 2 - Turing Engine PS2
- [ ] Desarrollar las utilidades del motor del juego para cumplir los requisitos minimos de las historias de usuario.
    - [x] Desarrollar un sistema básico de calculos vectoriales 2D.
        - [X] Crear un directorio llamado libs en el directorio include.
        - [X] Crear un directorio llamado math dentro del directorio libs.
        - [X] Crear un fichero de cabecera llamado vectorial.h
        - [X] Crear una estructura vec2f, dicha estructura se utilizará para representar posiciones, direcciones, velocidades, coordenadas de textura, etc.
        - [x] Crear el set de funciones para realizar operaciones ariméticas y vectoriale que reciban 2 vectores vec2f y generen el resultado en un tercer vector:
            - [x] Suma.
            - [x] Resta.
            - [x] Multiplicación.
            - [x] Multiplicar un vector por un escalar.
            - [x] Calcular el producto punto.
            - [x] Calcular la magnitud cuadrática de un vector.
            - [x] Calcular la magnitud de un vector.
            - [x] Normalizar un vector.
            - [x] Interpolación lineal entre 2 vectores.
        - [X] Hacer que la librería sea utilizable desde cualquier fichero fuente .c.
    - [x] Crear una librería render que ofreca las siguientes funcionalidades:
        - [x] Inicializar el motor de vídeo a través de dmaKit y gsKit.
        - [x] Crear texturas GS de 32BPP ABGR subiendo sus datos a la VRAM.
        - [x] Crear texturas GS en formato CI8 con pixeles de 8BPP y paleta de 256 colores subiendo sus datos a la VRAM.
        - [x] Sistema de registro de quads a dibujar por lotes (batching rendering).
            - [x] Registrar un RenderQuad con sus propiedades de posición, ancho, alto, coordenadas de textura, ID GS de textura en VRAM, flip de textura horizontal y vertical, y color de máscara.
            - [x] Al registrar un RenderQuad se debe de sumar +1 a un contador que empieza desde 0 por frame de modo que se van agregando los RenderQuad a dibujar en el frame.
            - [x] Capacidad para reestablecer el contador de RenderQuads dibujados en el frame anterior a 0 preparando el sistema para el siguiente frame.
            - [x] Mandar a dibujar todos los quads a través de gsKit.
        - [x] Liberar recursos utilizados.
        - [x] Gestión de colores y propiedades del sistema de vídeo.
    - [x] Implementar el sistema de inicialización y captura de inputs del control.
    - [x] Crear una librería que ayude a calcular el tiempo que pasa entre frame y frame (deltatime).
    - [x] Implementar un sistema flexible y escalable para gestionar los eventos de control.
    - [x] Implementar el sistema de sprites y sus animaciones:
       - [x] Crear estructura `Sprite` que contenga:
           - [x] La instancia de la estructura de la textura subida a la VRAM.
           - [x] La cantidad de columnas del sprite en caso de ser una hoja de sprites (`spritesheet`).
           - [x] La cantidad de filas del sprite en caso de ser una hoja de sprites (`spritesheet`).
           - [x] El ancho en píxeles por celda de la hoja de sprites.
           - [x] El alto en píxeles por celda de la hoja de sprites.
       - [x] Implementar el subsistema de animaciones de sprites (`SpriteAnimation` y `SpriteFrame`):
           - [x] Configuración de rangos de celdas, velocidades de reproducción y comportamiento de bucle (`loop`).
           - [x] Soporte para inversión de orientación horizontal/vertical (`flip_x`, `flip_y`).
           - [x] Cálculo dinámico de coordenadas UV (`u1, v1, u2, v2`) por fotograma en base a la cuadrícula.
           - [x] Definición e inicialización de estados de animación del jugador (idle y caminar a izquierda/derecha).
           - [x] Encapsulamiento de la lógica de selección y avance de animación (`update_animation`).
    - [x] Implementar un sistema de máquina de estados finita (FSM) basada en cabecera única (*header-only*).
        - [x] Diseñar estructura de tamaño fijo sin asignación dinámica (`malloc`/`free`) ideal para sistemas embebidos y retro (PS2SDK).
        - [x] Soportar ciclo de vida completo por estado con ganchos: `enter`, `step` y `close`.
        - [x] Implementar la función de transición de estados (`fsm_change`) con ejecución automática de limpieza y entrada.
        - [x] Implementar la función de ejecución de bucle principal (`fsm_step`).
        - [x] Añadir soporte para contexto personalizado de datos (`StateContext` / `void*`) para evitar variables globales.
        - [x] Proveer funciones `static inline` para evitar errores de enlace múltiple (*linker errors*).
        - [x] Documentar completamente el código con estándares de Doxygen en inglés, incluyendo casos de uso prácticos.


## Fase 3 - Entidades
- [ ] Desarrollar lo minimo necesario del sistema de entidades para crear una estructura Player mediante la cual podamos controlar a una entidad jugador.
    - [ ] Crear una estructura Entity.
    - [ ] Crear una estructura de física.    
    - [ ] Desarrollar una máquina de estados para las entidades.
    - [ ] Darle el aspecto visual a las entidades, colocarle un sprite del SA-X en estado idle al personaje.
    - [ ] Animar al personaje, hacer que se reproduzcan animaciones de sprites del personaje.
    - [ ] Reproducir animaciones en relación al estado del personaje.

## Conceptos

### ¿Qué es un tile?

Bajo el contexto de los videojuegos 2D podemos entender un tile/mosaico como una celda en una grilla o tabla la cual tiene propiedades especiales, estas propiedades definen su aspecto visual (textura), como reacciona cuando otros objetos interactuan con esta celda (colisiones), sus fisicas (si el tile tiene alguna componente de fisica) y como evoluciona a medida que fluye el juego. 

### Historia de colisiones

El jugador puede moverse a través del mapa, saltar a plataformas más altas abrir puertas a otras partes del mapa, 



