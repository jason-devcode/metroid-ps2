EE_BIN = builds/metroid.elf

# Detección dinámica y recursiva de archivos .c dentro de src/ y subcarpetas
SRC_DIRS = src
C_FILES = $(shell find $(SRC_DIRS) -type f -name '*.c')

# Ruta de inclusión local (usando CURDIR para evitar errores de sintaxis en el shell)
INCLUDE_LIBS = $(CURDIR)/include

# Mapear los archivos .c a objetos .o dentro de una carpeta separada (obj/)
OBJ_DIR = obj
EE_OBJS = $(patsubst %.c, $(OBJ_DIR)/%.o, $(C_FILES))

# Rutas de inclusión de cabeceras, añadiendo -MMD -MP para generar dependencias automáticas
EE_CFLAGS += -O3 -MMD -MP -I$(INCLUDE_LIBS) -I$(PS2SDK)/ports/include -I$(PS2SDK)/ports/include/SDL -I$(GSKIT)/include

# Rutas de librerías estáticas
EE_LDFLAGS += -L$(PS2SDK)/ports/lib -L$(GSKIT)/lib

# Enlace de librerías
EE_LIBS = -lSDL -lgskit -lgskit_toolkit -ldmakit -laudsrv -lmath3d -ldraw -lgraph -lc -lm

all: $(EE_BIN)

# Regla para compilar objetos asegurando que se creen las subcarpetas necesarias en 'obj/'
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

# Usamos dependencias normales para que el SDK maneje el binario final sin warnings
$(EE_BIN): $(EE_OBJS)
	@mkdir -p $(dir $@)
	$(EE_CC) -o $@ $(EE_OBJS) $(EE_LDFLAGS) $(EE_LIBS)
	$(EE_STRIP) --strip-all $@

# Incluir automáticamente los archivos de dependencia (.d) generados por GCC
-include $(EE_OBJS:.o=.d)

clean:
	rm -rf $(EE_BIN) $(OBJ_DIR)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal