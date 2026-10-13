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

## Fase 2 - Personaje

- [ ] Desarrollar lo minimo necesario del sistema de entidades para crear una estructura Player mediante la cual podamos controlar a una entidad jugador.
    - [x] Crear una estructura Entity.
    
- [ ] Desarrollar una máquina de estados para las entidades.
- [ ] Darle el aspecto visual a las entidades, colocarle un sprite del SA-X en estado idle al personaje.
- [ ] Animar al personaje, hacer que se reproduzcan animaciones de sprites del personaje.
- [ ] Reproducir animaciones en relación al estado del personaje.

## Conceptos

### ¿Qué es un tile?

Bajo el contexto de los videojuegos 2D podemos entender un tile/mosaico como una celda en una grilla o tabla la cual tiene propiedades especiales, estas propiedades definen su aspecto visual (textura), como reacciona cuando otros objetos interactuan con esta celda (colisiones), sus fisicas (si el tile tiene alguna componente de fisica) y como evoluciona a medida que fluye el juego. 

### ¿Qué es un tilemap?

Un tilemap o mapa de mosaicos es la grilla de NxM mosaicos donde se coloca cada mosaico.


