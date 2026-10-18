#!/usr/bin/env bash
set -Eeuo pipefail

# =============================================================================
# Configuración del proyecto
# =============================================================================
PROJECT_NAME="metroid-ps2"
IMAGE_NAME="metroid-ps2-builder"
DOCKERFILE="Dockerfile"
OUTPUT_DIR="builds"
OUTPUT_ELF_NAME="metroid.elf"

# Opciones de compilación y emulación
MAKE_TARGET=""
ELF_SOURCE=""
RUN_EMULATOR=true  # Activar o desactivar intento de lanzamiento

DOCKER_BUILD_ARGS=()
DOCKER_RUN_ARGS=()

# =============================================================================
# Lógica del script
# =============================================================================
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR"
DOCKERFILE_PATH="$PROJECT_DIR/$DOCKERFILE"
OUTPUT_PATH="$PROJECT_DIR/$OUTPUT_DIR"
CONTAINER_WORKDIR="/src"

# Marcador de caché para la huella digital (hash) del Dockerfile
BUILD_CACHE_MARKER="$OUTPUT_PATH/.docker_image_hash"

log() {
    printf '\n[%s] %s\n' "$PROJECT_NAME" "$*"
}

die() {
    printf '\nERROR: %s\n' "$*" >&2
    exit 1
}

command -v docker >/dev/null 2>&1 || die "Docker no está instalado o no está en PATH."
docker info >/dev/null 2>&1 || die "No se puede acceder al daemon de Docker."
[[ -f "$DOCKERFILE_PATH" ]] || die "No existe el Dockerfile: $DOCKERFILE_PATH"
[[ -f "$PROJECT_DIR/Makefile" ]] || die "No existe Makefile en: $PROJECT_DIR"

mkdir -p "$OUTPUT_PATH"

# =============================================================================
# Comprobación de cambios en el Dockerfile / Imagen
# =============================================================================
CURRENT_DOCKERFILE_HASH="$(sha256sum "$DOCKERFILE_PATH" | awk '{print $1}')"
IMAGE_EXISTS="$(docker image inspect "$IMAGE_NAME" >/dev/null 2>&1 && echo "yes" || echo "no")"
SAVED_HASH=""
[[ -f "$BUILD_CACHE_MARKER" ]] && SAVED_HASH="$(cat "$BUILD_CACHE_MARKER")"

if [[ "$IMAGE_EXISTS" == "no" ]] || [[ "$CURRENT_DOCKERFILE_HASH" != "$SAVED_HASH" ]]; then
    log "Cambios detectados en el Dockerfile o imagen ausente. Reconstruyendo imagen: $IMAGE_NAME"
    docker build \
        --tag "$IMAGE_NAME" \
        --file "$DOCKERFILE_PATH" \
        "${DOCKER_BUILD_ARGS[@]}" \
        "$PROJECT_DIR"
    
    echo "$CURRENT_DOCKERFILE_HASH" > "$BUILD_CACHE_MARKER"
else
    log "La imagen Docker ya está actualizada."
fi

# =============================================================================
# Compilación en el contenedor
# =============================================================================
log "Compilando el proyecto dentro del contenedor"
# =============================================================================
# Compilación en el contenedor (con el usuario actual para evitar problemas de permisos)
# =============================================================================
log "Compilando el proyecto dentro del contenedor"
docker run --rm \
    --user "$(id -u):$(id -g)" \
    --name "${IMAGE_NAME}-build" \
    --volume "$PROJECT_DIR:$CONTAINER_WORKDIR" \
    --workdir "$CONTAINER_WORKDIR" \
    "${DOCKER_RUN_ARGS[@]}" \
    "$IMAGE_NAME" \
    sh -lc '
        export PATH="$PATH:/usr/local/ps2dev/bin:/usr/local/ps2dev/ee/bin:/usr/local/ps2dev/iop/bin:/usr/local/ps2dev/dvp/bin:/usr/local/ps2dev/ps2sdk/bin"
        set -eu
        if [ -n "$1" ]; then
            make "$1"
        else
            make
        fi
    ' sh "$MAKE_TARGET"

# =============================================================================
# Procesamiento de artefactos (.elf)
# =============================================================================
DESTINATION_PATH="$OUTPUT_PATH/$OUTPUT_ELF_NAME"

[[ -f "$DESTINATION_PATH" ]] || die "La compilación terminó, pero no se encontró el ELF en: $DESTINATION_PATH"
[[ -s "$DESTINATION_PATH" ]] || die "El ELF de salida está vacío: $DESTINATION_PATH"

log "¡Compilación completada correctamente!"
printf 'ELF generado: %s\n' "$DESTINATION_PATH"
ls -lh -- "$DESTINATION_PATH"


# =============================================================================
# Detección y lanzamiento inteligente en PCSX2
# =============================================================================
if [[ "$RUN_EMULATOR" == "true" ]]; then
    PCSX2_BIN=""

    # 1. Buscar en rutas alternativas y conocidas del sistema
    if [[ -x "/usr/games/PCSX2" ]]; then
        PCSX2_BIN="/usr/games/PCSX2"
    elif command -v pcsx2 >/dev/null 2>&1; then
        PCSX2_BIN="$(command -v pcsx2)"
    elif flatpak info net.pcsx2.PCSX2 >/dev/null 2>&1; then
        PCSX2_BIN="flatpak"
    fi

    # 2. Ejecutar si se encontró un método válido, de lo contrario descartar
    if [[ -n "$PCSX2_BIN" ]]; then
        log "Iniciando PCSX2 con el ELF generado..."
        if [[ "$PCSX2_BIN" == "flatpak" ]]; then
            flatpak run net.pcsx2.PCSX2 -nogui -elf "$DESTINATION_PATH" >/dev/null 2>&1 &
        else
            "$PCSX2_BIN" -nogui -elf "$DESTINATION_PATH" >/dev/null 2>&1 &
        fi
    else
        log "Aviso: No se encontró PCSX2 en el sistema. Se omite el lanzamiento del emulador."
    fi
fi