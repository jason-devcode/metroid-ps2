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
        - [x] Inicializar el motor de vídeo de SDL.
        - [x] Desactivar el cursor en pantalla.
        - [x] Inicializar el framebuffer.
        - [x] Dibujar el framebuffer a travé de Doble-Buffering.
        - [x] Liberar recursos utilizados.
        - [x] Gestión de colores y propiedades del sistema de vídeo.
    - [x] Crear una librería que ofrezca las utilidades necesarias para calcular el deltatime de cada frame.

## Fase 3 - Entidades
- [ ] Desarrollar lo minimo necesario del sistema de entidades para crear una estructura Player mediante la cual podamos controlar a una entidad jugador.
    - [x] Crear una estructura Entity.
    - [ ] Crear una estructura de física.    
    - [ ] Desarrollar una máquina de estados para las entidades.
    - [ ] Darle el aspecto visual a las entidades, colocarle un sprite del SA-X en estado idle al personaje.
    - [ ] Animar al personaje, hacer que se reproduzcan animaciones de sprites del personaje.
    - [ ] Reproducir animaciones en relación al estado del personaje.

## Conceptos

### ¿Qué es un tile?

Bajo el contexto de los videojuegos 2D podemos entender un tile/mosaico como una celda en una grilla o tabla la cual tiene propiedades especiales, estas propiedades definen su aspecto visual (textura), como reacciona cuando otros objetos interactuan con esta celda (colisiones), sus fisicas (si el tile tiene alguna componente de fisica) y como evoluciona a medida que fluye el juego. 

### ¿Qué es un tilemap?

Un tilemap o mapa de mosaicos es la grilla de NxM mosaicos donde se coloca cada mosaico.


