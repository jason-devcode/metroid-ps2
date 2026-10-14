EE_BIN = builds/metroid.elf

# Detección dinámica y recursiva de archivos .c dentro de src/ y subcarpetas
SRC_DIRS = src
C_FILES = $(shell find $(SRC_DIRS) -type f -name '*.c')

# Ruta de inclusión local
INCLUDE_LIBS = $(CURDIR)/include

# Mapear los archivos .c a objetos .o dentro de una carpeta separada (obj/)
OBJ_DIR = obj
EE_OBJS = $(patsubst %.c, $(OBJ_DIR)/%.o, $(C_FILES))

# Definición de herramientas del Toolchain de PS2
EE_CC = mips64r5900el-ps2-elf-gcc
EE_STRIP = mips64r5900el-ps2-elf-strip

# Flags de compilación e inclusión de cabeceras
EE_CFLAGS = -D_EE -G0 -O2 -Wall -Wno-unused-function -gdwarf-2 -gz -MMD -MP \
            -I$(INCLUDE_LIBS) \
            -I$(PS2SDK)/ports/include \
            -I$(PS2SDK)/ports/include/SDL \
            -I$(GSKIT)/include \
            -I$(PS2SDK)/ee/include \
            -I$(PS2SDK)/common/include

# Rutas de librerías estáticas
EE_LDFLAGS = -L$(PS2SDK)/ee/lib -L$(PS2SDK)/ports/lib -L$(GSKIT)/lib -Wl,-zmax-page-size=128

# Enlace de librerías
EE_LIBS = -lSDL -lgskit -lgskit_toolkit -ldmakit -laudsrv -lmath3d -ldraw -lgraph -lc -lm -lcdvd -lkernel

all: $(EE_BIN)

# Regla para compilar objetos asegurando que se creen las subcarpetas necesarias en 'obj/'
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(EE_CC) $(EE_CFLAGS) -c $< -o $@

# Enlace final explícito con las rutas exactas dentro del contenedor
$(EE_BIN): $(EE_OBJS)
	@mkdir -p $(dir $@)
	$(EE_CC) -nostartfiles -T/usr/local/ps2dev/ps2sdk/ee/startup/linkfile \
		-o $@ /usr/local/ps2dev/ee/mips64r5900el-ps2-elf/lib/crt0.o $(EE_OBJS) \
		$(EE_LDFLAGS) $(EE_LIBS)
	$(EE_STRIP) --strip-all $@

# Incluir automáticamente los archivos de dependencia (.d) generados por GCC
-include $(EE_OBJS:.o=.d)

clean:
	rm -rf $(EE_BIN) $(OBJ_DIR)

# Solo cargamos Makefile.pref para tener las variables de rutas del SDK, evitando que eeglobal rompa nuestro enlace
include $(PS2SDK)/samples/Makefile.pref