# ==============================================================================
# Base Image
# ==============================================================================
# Uses the official PS2Dev container image, which contains the pre-compiled 
# MIPS cross-compilers for the PS2 hardware (EE, IOP, DVP).
FROM ghcr.io/ps2dev/ps2dev:latest

# ==============================================================================
# System Dependencies
# ==============================================================================
# Installs core build utilities on top of the base Alpine Linux system.
# --no-cache: Prevents caching package indexes to minimize image size.
RUN apk add --no-cache \
    build-base \
    git \
    cmake \
    bash \
    patch \
    zip \
    gawk

# ==============================================================================
# Environment Variables (PS2 Toolchain & SDK)
# ==============================================================================
# Core paths for the PS2Dev ecosystem
ENV PS2DEV=/usr/local/ps2dev
ENV PS2SDK=$PS2DEV/ps2sdk
ENV GSKIT=$PS2DEV/gsKit

# Appends MIPS cross-compilers and tools to the system PATH
ENV PATH=$PATH:$PS2DEV/bin:$PS2DEV/ee/bin:$PS2DEV/iop/bin:$PS2DEV/dvp/bin:$PS2SDK/bin

# ==============================================================================
# SDL Port Compilation & Installation
# ==============================================================================
# Descarga, compila e instala estrictamente el puerto de SDL, creando un enlace
# simbólico para asegurar la compatibilidad del nombre con -lSDL (mayúsculas).
RUN mkdir -p /tmp/ps2sdk-ports && \
    cd /tmp/ps2sdk-ports && \
    git init --quiet && \
    git remote add origin https://github.com/ps2dev/ps2sdk-ports.git && \
    git config core.sparseCheckout true && \
    echo "sdl" >> .git/info/sparse-checkout && \
    git pull --depth 1 --quiet origin master && \
    cd sdl && \
    make clean && \
    make && \
    make install && \
    if [ -f /usr/local/ps2dev/ps2sdk/ports/lib/libsdl.a ] && [ ! -f /usr/local/ps2dev/ps2sdk/ports/lib/libSDL.a ]; then \
        ln -s libsdl.a /usr/local/ps2dev/ps2sdk/ports/lib/libSDL.a; \
    fi && \
    rm -rf /tmp/ps2sdk-ports

# ==============================================================================
# Working Directory
# ==============================================================================
# Sets the default working directory where local source code will be mounted.
WORKDIR /src