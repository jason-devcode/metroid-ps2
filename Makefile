# ============================================================
# CONFIGURACIÓN DEL PROYECTO
# ============================================================

EE_BIN = builds/metroid.elf

# Directorios fuente: búsqueda recursiva de archivos .c
SRC_DIRS = src
C_FILES = $(shell find $(SRC_DIRS) -type f -name '*.c' | sort)

# Directorio de cabeceras locales
INCLUDE_LIBS = $(CURDIR)/include

# Directorio de objetos
OBJ_DIR = obj
EE_OBJS = $(patsubst %.c,$(OBJ_DIR)/%.o,$(C_FILES))

# ============================================================
# CONFIGURACIÓN DE OPTIMIZACIÓN
# ============================================================

OPT_LEVEL = -O3

# LTO: optimización entre archivos durante el enlace
ENABLE_LTO = 1

# Matemáticas de coma flotante agresivas
# 0 = desactivado; 1 = activado
ENABLE_FAST_MATH = 1

# ============================================================
# TOOLCHAIN ORIGINAL DE PS2
# ============================================================

EE_CC = mips64r5900el-ps2-elf-gcc
EE_STRIP = mips64r5900el-ps2-elf-strip

# ============================================================
# FLAGS DE COMPILACIÓN
# ============================================================

EE_CFLAGS = \
    -D_EE \
    -G0 \
    $(OPT_LEVEL) \
    -fomit-frame-pointer \
    -fno-math-errno \
    -ffunction-sections \
    -fdata-sections \
    -DNDEBUG \
    -Wall \
    -Wno-unused-function \
    -gdwarf-2 \
    -gz \
    -MMD \
    -MP \
    -I$(INCLUDE_LIBS) \
    -I$(PS2SDK)/ports/include \
    -I$(PS2SDK)/ports/include/SDL \
    -I$(GSKIT)/include \
    -I$(PS2SDK)/ee/include \
    -I$(PS2SDK)/common/include

ifeq ($(ENABLE_LTO),1)
    EE_CFLAGS += -flto
endif

ifeq ($(ENABLE_FAST_MATH),1)
    EE_CFLAGS += -ffast-math
endif

# ============================================================
# RUTAS DE LIBRERÍAS ORIGINALES
# ============================================================

EE_LDFLAGS = \
    -L$(PS2SDK)/ee/lib \
    -L$(PS2SDK)/ports/lib \
    -L$(GSKIT)/lib \
    -Wl,-zmax-page-size=128 \
    -Wl,--gc-sections

# ============================================================
# LIBRERÍAS ORIGINALES
# ============================================================

EE_LIBS = \
    -lSDL \
    -lgskit \
    -lgskit_toolkit \
    -ldmakit \
    -laudsrv \
    -lmath3d \
    -ldraw \
    -lgraph \
    -lc \
    -lm \
    -lcdvd \
    -lkernel

# ============================================================
# OBJETIVO PRINCIPAL
# ============================================================

.PHONY: all clean

all: $(EE_BIN)

# ============================================================
# COMPILACIÓN DE ARCHIVOS FUENTE
# ============================================================

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(EE_CC) $(EE_CFLAGS) -c $< -o $@

# ============================================================
# ENLACE DEL EJECUTABLE
# ============================================================

$(EE_BIN): $(EE_OBJS)
	@mkdir -p $(dir $@)
	$(EE_CC) \
	    -nostartfiles \
	    -T/usr/local/ps2dev/ps2sdk/ee/startup/linkfile \
	    $(if $(filter 1,$(ENABLE_LTO)),-flto,) \
	    -o $@ \
	    /usr/local/ps2dev/ee/mips64r5900el-ps2-elf/lib/crt0.o \
	    $(EE_OBJS) \
	    $(EE_LDFLAGS) \
	    $(EE_LIBS)
	$(EE_STRIP) --strip-all $@

# ============================================================
# DEPENDENCIAS AUTOMÁTICAS
# ============================================================

-include $(EE_OBJS:.o=.d)

# ============================================================
# LIMPIEZA
# ============================================================

clean:
	rm -rf $(EE_BIN) $(OBJ_DIR)

# ============================================================
# VARIABLES DE RUTA DEL PS2SDK
# ============================================================

include $(PS2SDK)/samples/Makefile.pref