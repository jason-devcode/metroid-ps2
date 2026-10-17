"""
================================================================================
SCRIPT DE CONVERSIÓN Y OPTIMIZACIÓN DE SPRITESHEETS PARA PLAYSTATION 2 (PS2)
================================================================================

AUTOR: Colaborador AI / Desarrollador PS2
DESCRIPCIÓN:
    Toma un spritesheet PNG grande (1280x1700 píxeles, compuesto por una rejilla 
    de 32 columnas y 34 filas, donde cada tile mide 40x50 píxeles), extrae una 
    subsección personalizada mediante rangos de columnas e índices específicos 
    de filas, y lo convierte en un archivo de cabecera en C (.h).

DECISIONES TÉCNICAS Y POR QUÉ FUNCIONA ASÍ EN LA PS2:

1. FORMATO CI8 (Color Indexed 8-bit):
   - Por qué: La PS2 cuenta con una memoria VRAM extremadamente limitada (4 MB en total). 
     Guardar texturas en color real RGBA32 (4 bytes por píxel) satura la VRAM de inmediato. 
     Con CI8, cada píxel se almacena como un índice numérico de 8 bits (1 byte), 
     reduciendo drásticamente el consumo de memoria en un 75% y facilitando el 
     ancho de banda del bus DMA.

2. TRANSPARENCIA BINARIA Y SOPORTE DE ALPHA TESTING (GS):
   - Por qué: Aunque se descartó el canal alpha completo para ahorrar espacio, 
     se mantiene una transparencia binaria (1 bit). En la paleta RGBA32 de la PS2, 
     los píxeles totalmente transparentes del fondo se configuran con Alpha = 0x00, 
     mientras que los opacos reciben Alpha = 0x80 (estándar del GS). Esto permite 
     activar el registro de Alpha Test (ATE) en el Graphics Synthesizer de la PS2, 
     haciendo que el hardware descarte el fondo automáticamente sin consumir 
     ciclos de CPU.

3. PALETA GLOBAL DE 256 COLORES (CLUT):
   - Por qué: El formato CI8 requiere una tabla de colores (Color Look-Up Table). 
     El script escanea los píxeles seleccionados, extrae los colores únicos 
     (limitados estrictamente a un máximo de 256) y empaqueta cada color en un 
     entero de 32 bits con formato 0xRRGGBBAA, listo para ser transferido a la 
     VRAM mediante el canal GIF/DMA.

4. ALINEACIÓN A 16 BYTES (__attribute__((aligned(16)))):
   - Por qué: La arquitectura del Emotion Engine (EE) y el Graphics Interface (GIF) 
     de la PS2 procesa los datos en bloques de 128 bits (Qwords / 16 bytes). 
     Alinear los arrays en C con esta directiva evita fallos de alineación de memoria 
     durante las transferencias DMA ultrarrápidas.

5. SELECCIÓN DINÁMICA DE FILAS Y COLUMNAS:
   - Por qué: Permite exportar únicamente las animaciones o tiles específicos 
     que el juego necesita en un momento dado, evitando cargar datos basura en la 
     memoria de la consola.
================================================================================
"""

import sys
from PIL import Image

def export_custom_spritesheet_ci8_ps2(image_path, output_header_path, start_col, end_col, selected_rows):
    """
    Función principal que procesa la imagen PNG, aplica el filtrado de región,
    calcula la paleta CI8 con alpha y escribe el archivo .h optimizado.
    """
    try:
        # Abrimos la imagen original asegurando el modo RGBA para capturar 
        # correctamente los canales de color y la transparencia del fondo.
        img = Image.open(image_path).convert("RGBA")
    except Exception as e:
        print(f"Error crítico al abrir la imagen: {e}")
        return

    full_width, full_height = img.size
    tile_w, tile_h = 40, 50
    total_cols, total_rows = 32, 34

    # 1. Validación de rangos de columnas para evitar desbordamientos
    start_col = max(0, min(start_col, total_cols - 1))
    end_col = max(start_col, min(end_col, total_cols - 1))
    num_cols = (end_col - start_col) + 1

    # 2. Validación de las filas específicas solicitadas por el usuario
    valid_rows = [r for r in selected_rows if 0 <= r < total_rows]
    if not valid_rows:
        print("Error: No se han seleccionado filas válidas (deben estar entre 0 y 33).")
        return
    
    num_rows = len(valid_rows)
    sub_width = num_cols * tile_w
    sub_height = num_rows * tile_h

    print(f"\n[INFO] Procesando sub-spritesheet optimizado para PS2:")
    print(f" -> Columnas (Rango): {start_col} a {end_col} ({num_cols} columnas)")
    print(f" -> Filas (Específicas): {valid_rows} ({num_rows} filas en total)")
    print(f" -> Resolución resultante: {sub_width}x{sub_height} píxeles")

    # 3. Extracción de píxeles tile por tile según el orden de las filas seleccionadas
    sub_pixels = []
    for r in valid_rows:
        for py in range(tile_h):
            global_y = (r * tile_h) + py
            for c in range(start_col, end_col + 1):
                for px in range(tile_w):
                    global_x = (c * tile_w) + px
                    r_val, g_val, b_val, a_val = img.getpixel((global_x, global_y))
                    
                    # Normalización de transparencia para el hardware de la PS2 (GS):
                    # Si el píxel es semi/completamente transparente (alpha <= 128), 
                    # asignamos 0x00 para que el Alpha Test lo ignore. 
                    # Si es opaco, asignamos 0x80 (estándar en PS2).
                    ps2_alpha = 0xFF if a_val > 128 else 0x00
                    sub_pixels.append((r_val, g_val, b_val, ps2_alpha))

    # 4. Generación de la Paleta CI8 (Color Look-Up Table)
    # Buscamos colores únicos y mapeamos cada píxel a un índice numérico (0 a 255).
    palette = []
    color_to_index = {}
    indices = []

    for p in sub_pixels:
        if p not in color_to_index:
            if len(palette) >= 256:
                print("Error: La selección excede los 256 colores únicos permitidos para el formato CI8.")
                return
            color_to_index[p] = len(palette)
            palette.append(p)
        indices.append(color_to_index[p])

    num_colors = len(palette)
    print(f"[INFO] Paleta generada con éxito: {num_colors} colores únicos (Formato CI8).")

    # 5. Escritura del archivo de cabecera de C (.h)
    with open(output_header_path, "w") as f:
        guard_name = output_header_path.upper().replace(".", "_").replace("/", "_").replace("\\", "_")
        
        # Guardas de inclusión estándar para evitar múltiples definiciones
        f.write(f"#ifndef {guard_name}\n")
        f.write(f"#define {guard_name}\n\n")
        f.write("#include <stdint.h>\n\n")

        # Macros de metadatos útiles para dimensionar las texturas en el código de la PS2
        f.write(f"#define SUB_SPRITESHEET_WIDTH   {sub_width}\n")
        f.write(f"#define SUB_SPRITESHEET_HEIGHT  {sub_height}\n")
        f.write(f"#define SUB_SPRITE_COLS       {num_cols}\n")
        f.write(f"#define SUB_SPRITE_ROWS       {num_rows}\n")
        f.write(f"#define SUB_TILE_WIDTH        {tile_w}\n")
        f.write(f"#define SUB_TILE_HEIGHT       {tile_h}\n")
        f.write(f"#define SUB_PALETTE_SIZE      {num_colors}\n\n")

        # Definición de la Paleta CI8 (RGBA32)
        # Se usa '__attribute__((aligned(16)))' para cumplir con el requisito de 
        # alineación Qword del DMA de la PS2.
        f.write(f"// Paleta de colores CI8 con soporte de Alpha Test para el GS de la PS2\n")
        f.write(f"static const uint32_t spritesheet_palette[{num_colors}] __attribute__((aligned(16))) = {{\n")
        for i, (r, g, b, a) in enumerate(palette):
            # Empaquetado nativo en formato ABGR para la PS2: (Alpha << 24) | (Blue << 16) | (Green << 8) | Red
            val = (a << 24) | (b << 16) | (g << 8) | r
            f.write(f"0x{val:08X}, ")
            if (i + 1) % 8 == 0:
                f.write("\n")

        f.write("\n};\n\n")

        # Definición del Array de Índices (1 byte por píxel)
        # Cada valor apunta directamente a una posición de la paleta anterior.
        f.write(f"// Array de índices de píxeles en formato decimal ({sub_width * sub_height} bytes, alineado a 16 bytes)\n")
        f.write(f"static const uint8_t spritesheet_indices[{sub_width * sub_height}] __attribute__((aligned(16))) = {{\n")
        for i, idx in enumerate(indices):
            f.write(f"{idx}, ")
            if (i + 1) % 16 == 0:
                f.write("\n")
        f.write("\n};\n\n")

        f.write(f"#endif // {guard_name}\n")

    print(f"[EXITO] Archivo de cabecera generado correctamente en '{output_header_path}'")

if __name__ == "__main__":
    print("================================================================")
    print("  Conversor Interactivo de Spritesheet CI8 (Optimizado para PS2) ")
    print("================================================================")
    
    try:
        # Solicitud interactiva de parámetros al usuario
        s_col = int(input("Introduce la columna inicial (0 a 31): "))
        e_col = int(input("Introduce la columna final (0 a 31): "))
        
        rows_input = input("Introduce los índices de las filas separados por comas (ej: 0, 2, 5, 12): ")
        selected_rows = [int(r.strip()) for r in rows_input.split(",") if r.strip().isdigit()]
        
    except ValueError:
        print("[ERROR] Formato de entrada no válido. Debes introducir números enteros.")
        sys.exit(1)

    # Ejecución de la conversión con los parámetros provistos
    export_custom_spritesheet_ci8_ps2("spritesheet.png", "spritesheet_ci8.h", s_col, e_col, selected_rows)