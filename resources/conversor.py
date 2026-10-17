"""
================================================================================
CONVERSOR DE SPRITESHEETS CI8 PARA PLAYSTATION 2 (ARQUITECTURA SOLID)
================================================================================

Este módulo provee clases modulares para la carga, procesamiento, conversión de
paleta (CSM1), indexación de colores y exportación a archivos de cabecera C
optimizados para la arquitectura de la PlayStation 2.
================================================================================
"""

import sys
from abc import ABC, abstractmethod
from typing import List, Tuple, Dict, Sequence
from PIL import Image


# ==============================================================================
# CONSTANTES DE CONFIGURACIÓN Y HARDWARE PS2
# ==============================================================================
# El Graphics Synthesizer (GS) de PS2 requiere que los anchos de textura estén
# alineados a múltiplos de 64 píxeles para optimizar las transferencias DMA y 
# el almacenamiento en VRAM.
PS2_TEXTURE_WIDTH_ALIGNMENT = 64

# Umbral de transparencia binaria (mitad del rango de 8 bits: 255 / 2).
# Se usa para convertir un canal alpha continuo en una máscara de 0 o 255 
# compatible con motores gráficos clásicos de PS2 que no soportan alpha suave por píxel en CI8.
ALPHA_THRESHOLD = 128
PS2_ALPHA_OPAQUE = 0xFF
PS2_ALPHA_TRANSPARENT = 0x00

# CI8 utiliza índices de 8 bits, lo que requiere una CLUT (Color Look-Up Table) 
# exactamente de 256 entradas.
PS2_CLUT_PALETTE_SIZE = 256

# Constantes de hardware para el modo CSM1 (Color Storage Mode 1):
# El GS organiza las paletas en bloques de hardware específicos. Para que la 
# tarjeta gráfica lea los colores de forma lineal sin artefactos, es necesario 
# reordenar físicamente las entradas de la paleta usando operaciones de bits.
CSM1_INDEX_MASK = 0x18     # Máscara binaria (00011000) para aislar los bits de control de bloque.
CSM1_INDEX_TARGET = 0x08   # Patrón objetivo (00001000) que activa la condición de intercambio.
CSM1_SWAP_OFFSET = 8       # Distancia de desplazamiento del índice para el intercambio de bloques (i + 8).

# Alineación estricta requerida para DMA y el coprocesador vectorial (VU) de la PS2.
DMA_MEMORY_ALIGNMENT = 16


# ==============================================================================
# 1. INTERFACES Y CONTRATOS (Principios ISP y DIP)
# ==============================================================================

class IPaletteProcessor(ABC):
    """Interfaz abstracta para procesadores de paletas de color."""
    
    @abstractmethod
    def process(self, palette: List[Tuple[int, int, int, int]]) -> List[Tuple[int, int, int, int]]:
        """Procesa y reorganiza las entradas de la paleta."""
        pass


class ISpritesheetExtractor(ABC):
    """Interfaz abstracta para la extracción y alineación de sub-spritesheets."""
    
    @abstractmethod
    def extract(
        self, 
        image_path: str, 
        start_col: int, 
        end_col: int, 
        valid_rows: List[int]
    ) -> Tuple[List[Tuple[int, int, int, int]], int, int, int, int]:
        """Extrae los píxeles y calcula las dimensiones con padding."""
        pass


class IColorIndexer(ABC):
    """Interfaz abstracta para la indexación de colores y generación de paleta CI8."""
    
    @abstractmethod
    def index_pixels(
        self, 
        pixels: Sequence[Tuple[int, int, int, int]], 
        palette_size: int
    ) -> Tuple[List[Tuple[int, int, int, int]], List[int]]:
        """Mapea píxeles a una paleta de tamaño fijo y genera índices de 8 bits."""
        pass


class ICodeExporter(ABC):
    """Interfaz abstracta para la exportación de datos a archivos de cabecera C."""
    
    @abstractmethod
    def export(
        self,
        output_path: str,
        sub_width: int,
        sub_height: int,
        num_cols: int,
        num_rows: int,
        tile_w: int,
        tile_h: int,
        palette: List[Tuple[int, int, int, int]],
        indices: List[int]
    ) -> bool:
        """Exporta la paleta y los índices en formato C compatible con PS2."""
        pass


# ==============================================================================
# 2. IMPLEMENTACIONES CONCRETAS (Principio SRP)
# ==============================================================================

class CSM1PaletteProcessor(IPaletteProcessor):
    """
    Procesador de paletas específico para el formato CSM1 de PlayStation 2.
    Reorganiza las 256 entradas según el entrelazado de hardware requerido.
    """

    def process(self, palette: List[Tuple[int, int, int, int]]) -> List[Tuple[int, int, int, int]]:
        """
        Reorganiza las 256 entradas de la paleta para el almacenamiento CLUT CSM1.
        
        ¿Por qué se hace esto?
        El hardware de la PS2 mapea las texturas indexadas en VRAM de forma no lineal 
        cuando se opera en modo CSM1. Aplicar esta máscara de bits (0x18 y 0x08) 
        permite corregir el orden de los colores en bloques de 16/32 entradas 
        para que coincida exactamente con lo que el circuito gráfico espera renderizar.
        
        Args:
            palette (List[Tuple[int, int, int, int]]): Lista de colores RGBA.
            
        Returns:
            List[Tuple[int, int, int, int]]: Paleta reorganizada.
            
        Raises:
            ValueError: Si la paleta no contiene exactamente 256 entradas.
        """
        mutable_palette = list(palette)

        if len(mutable_palette) != PS2_CLUT_PALETTE_SIZE:
            raise ValueError(f"La paleta CSM1 debe contener exactamente {PS2_CLUT_PALETTE_SIZE} entradas.")

        # Recorremos la tabla completa de la paleta CI8 (256 colores)
        for i in range(PS2_CLUT_PALETTE_SIZE):
            # Evaluamos los bits mediante la máscara de hardware CSM1
            if (i & CSM1_INDEX_MASK) == CSM1_INDEX_TARGET:
                # Intercambiamos la entrada actual con su contraparte desplazada 
                # para alinear el bloque de memoria de la paleta en VRAM.
                mutable_palette[i], mutable_palette[i + CSM1_SWAP_OFFSET] = (
                    mutable_palette[i + CSM1_SWAP_OFFSET], 
                    mutable_palette[i]
                )

        return mutable_palette


class SpritesheetExtractor(ISpritesheetExtractor):
    """
    Extractor de sub-spritesheets desde una imagen fuente grande.
    Maneja validaciones de tamaño, selección de región y padding a múltiplos de 64.
    """

    def __init__(self, tile_w: int = 40, tile_h: int = 50, total_cols: int = 32, total_rows: int = 34):
        self.tile_w = tile_w
        self.tile_h = tile_h
        self.total_cols = total_cols
        self.total_rows = total_rows

    def extract(
        self, 
        image_path: str, 
        start_col: int, 
        end_col: int, 
        valid_rows: List[int]
    ) -> Tuple[List[Tuple[int, int, int, int]], int, int, int, int]:
        """
        Abre la imagen, valida dimensiones, extrae la región solicitada y aplica padding.
        
        Args:
            image_path (str): Ruta al archivo de imagen fuente.
            start_col (int): Columna inicial de la selección.
            end_col (int): Columna final de la selección.
            valid_rows (List[int]): Lista de filas seleccionadas y validadas.
            
        Returns:
            Tuple con (píxeles extraídos, ancho con padding, alto, número de columnas, número de filas).
        """
        img = Image.open(image_path).convert("RGBA")
        full_width, full_height = img.size

        required_width = self.total_cols * self.tile_w
        required_height = self.total_rows * self.tile_h

        if full_width < required_width or full_height < required_height:
            raise ValueError(
                f"La imagen es más pequeña que el spritesheet esperado.\n"
                f"        Tamaño recibido: {full_width}x{full_height}\n"
                f"        Tamaño mínimo esperado: {required_width}x{required_height}"
            )

        num_cols = end_col - start_col + 1
        num_rows = len(valid_rows)
        raw_sub_width = num_cols * self.tile_w

        # ¿Por qué se redondea el ancho a múltiplos de 64?
        # El ancho de la textura en VRAM debe cumplir estrictamente con los requisitos 
        # de alineación del bloque de almacenamiento del GS (Graphics Synthesizer) de PS2.
        sub_width = ((raw_sub_width + (PS2_TEXTURE_WIDTH_ALIGNMENT - 1)) // PS2_TEXTURE_WIDTH_ALIGNMENT) * PS2_TEXTURE_WIDTH_ALIGNMENT
        sub_height = num_rows * self.tile_h

        sub_pixels: List[Tuple[int, int, int, int]] = []

        # Extracción de teselas ordenadas por filas
        for row in valid_rows:
            for py in range(self.tile_h):
                global_y = row * self.tile_h + py

                for col in range(start_col, end_col + 1):
                    for px in range(self.tile_w):
                        global_x = col * self.tile_w + px

                        r, g, b, a = img.getpixel((global_x, global_y))

                        # ¿Por qué se evalúa un umbral de 128 para el canal alpha?
                        # Convierte una imagen RGBA estándar (con transparencia suave/antialiasing) 
                        # en una transparencia binaria estricta (0x00 o 0xFF), evitando artefactos 
                        # de borde en la GPU de la PlayStation 2 que no procesa alpha blending avanzado en CI8.
                        ps2_alpha = PS2_ALPHA_OPAQUE if a > ALPHA_THRESHOLD else PS2_ALPHA_TRANSPARENT
                        sub_pixels.append((r, g, b, ps2_alpha))

                # Rellenar el espacio sobrante del ancho con padding transparente negro (0,0,0,0)
                # para cumplir exactamente con el ancho alineado a múltiplos de 64.
                padding_width = sub_width - raw_sub_width
                for _ in range(padding_width):
                    sub_pixels.append((0, 0, 0, PS2_ALPHA_TRANSPARENT))

        return sub_pixels, sub_width, sub_height, num_cols, num_rows


class CI8ColorIndexer(IColorIndexer):
    """
    Indexador de colores para formato CI8 (8 bits por píxel).
    Genera una paleta única de hasta 256 colores y mapea los píxeles a índices.
    """

    def index_pixels(
        self, 
        pixels: Sequence[Tuple[int, int, int, int]], 
        palette_size: int = PS2_CLUT_PALETTE_SIZE
    ) -> Tuple[List[Tuple[int, int, int, int]], List[int]]:
        """
        Mapea una secuencia de píxeles RGBA a una paleta indexada y una lista de índices de 8 bits.
        
        Args:
            pixels (Sequence[Tuple[int, int, int, int]]): Lista de píxeles RGBA.
            palette_size (int): Tamaño fijo de la paleta (por defecto 256 para CI8).
            
        Returns:
            Tuple con (paleta completada, lista de índices uint8).
        """
        palette: List[Tuple[int, int, int, int]] = []
        color_to_index: Dict[Tuple[int, int, int, int], int] = {}
        indices: List[int] = []

        for pixel in pixels:
            index = color_to_index.get(pixel)

            if index is None:
                if len(palette) >= palette_size:
                    raise ValueError(
                        f"La selección contiene más de {palette_size} colores únicos. "
                        "No se puede representar en CI8 sin reducir la cantidad de colores."
                    )

                index = len(palette)
                color_to_index[pixel] = index
                palette.append(pixel)

            indices.append(index)

        # Rellenar entradas de paleta no utilizadas con transparencia completa (0,0,0,0)
        # para asegurar que la CLUT siempre contenga exactamente 256 entradas válidas.
        while len(palette) < palette_size:
            palette.append((0, 0, 0, PS2_ALPHA_TRANSPARENT))

        return palette, indices


class CHeaderCodeExporter(ICodeExporter):
    """
    Exportador de datos a archivos de cabecera en lenguaje C (.h).
    Escribe metadatos, la paleta empaquetada en 0xAABBGGRR y los índices alineados a 16 bytes.
    """

    def export(
        self,
        output_path: str,
        sub_width: int,
        sub_height: int,
        num_cols: int,
        num_rows: int,
        tile_w: int,
        tile_h: int,
        palette: List[Tuple[int, int, int, int]],
        indices: List[int]
    ) -> bool:
        """
        Escribe las estructuras de datos en el archivo de cabecera C especificado.
        
        Args:
            output_path (str): Ruta del archivo .h de salida.
            sub_width (int): Ancho corregido con padding.
            sub_height (int): Alto total de la textura.
            num_cols (int): Número de columnas exportadas.
            num_rows (int): Número de filas exportadas.
            tile_w (int): Ancho de cada tesela.
            tile_h (int): Alto de cada tesela.
            palette (List[Tuple[int, int, int, int]]): Paleta de 256 colores.
            indices (List[int]): Lista de índices de píxeles.
            
        Returns:
            bool: True si la exportación fue exitosa, False en caso contrario.
        """
        guard_name = (
            output_path.upper()
            .replace(".", "_")
            .replace("/", "_")
            .replace("\\", "_")
            .replace("-", "_")
        )

        try:
            with open(output_path, "w", encoding="utf-8") as file:
                file.write(f"#ifndef {guard_name}\n")
                file.write(f"#define {guard_name}\n\n")
                file.write("#include <stdint.h>\n\n")

                # Constantes de configuración exportadas al código C
                file.write(f"#define SUB_SPRITESHEET_WIDTH   {sub_width}\n")
                file.write(f"#define SUB_SPRITESHEET_HEIGHT  {sub_height}\n")
                file.write(f"#define SUB_SPRITE_COLS         {num_cols}\n")
                file.write(f"#define SUB_SPRITE_ROWS         {num_rows}\n")
                file.write(f"#define SUB_TILE_WIDTH          {tile_w}\n")
                file.write(f"#define SUB_TILE_HEIGHT         {tile_h}\n")
                file.write(f"#define SUB_PALETTE_SIZE        {len(palette)}\n\n")

                # Exportación de la paleta (Formato 0xAABBGGRR, alineado a 16 bytes)
                # ¿Por qué alineado a 16 bytes? El procesador EE (Emotion Engine) y los 
                # registros VU de la PS2 procesan bloques de 128 bits (quadwords) en paralelo mediante DMA.
                file.write(f"/* Paleta CI8 de {PS2_CLUT_PALETTE_SIZE} entradas, alineada a {DMA_MEMORY_ALIGNMENT} bytes. */\n")
                file.write(
                    f"static const uint32_t "
                    f"spritesheet_palette[SUB_PALETTE_SIZE] "
                    f"__attribute__((aligned({DMA_MEMORY_ALIGNMENT}))) = {{\n"
                )

                for i, (r, g, b, a) in enumerate(palette):
                    # Formato de color nativo de PS2 GS: 0xAABBGGRR
                    pal_alpha = 0x80 if a > ALPHA_THRESHOLD else 0x00
                    value = (pal_alpha << 24) | (b << 16) | (g << 8) | r
                    file.write(f"0x{value:08X},")

                    if (i + 1) % 8 == 0:
                        file.write("\n")
                    else:
                        file.write(" ")

                file.write("\n};\n\n")

                # Exportación de índices (uint8_t, alineado a 16 bytes para transferencia DMA óptima)
                file.write(f"/* Índices CI8: {len(indices)} bytes. */\n")
                file.write(
                    f"static const uint8_t "
                    f"spritesheet_indices[{len(indices)}] "
                    f"__attribute__((aligned({DMA_MEMORY_ALIGNMENT}))) = {{\n"
                )

                for i, index in enumerate(indices):
                    file.write(f"{index},")

                    if (i + 1) % 16 == 0:
                        file.write("\n")
                    else:
                        file.write(" ")

                file.write("\n};\n\n")
                file.write(f"#endif /* {guard_name} */\n")

        except OSError as error:
            print(f"[ERROR] No se pudo escribir la cabecera: {error}")
            return False

        return True


# ==============================================================================
# 3. CONTROLADOR PRINCIPAL (Principio de Inversión de Dependencias)
# ==============================================================================

class SpritesheetConverterController:
    """
    Controlador principal que coordina el flujo completo de conversión de spritesheets
    inyectando las dependencias correspondientes (Principio DIP).
    """

    def __init__(
        self,
        extractor: ISpritesheetExtractor,
        indexer: IColorIndexer,
        palette_processor: IPaletteProcessor,
        exporter: ICodeExporter,
        tile_w: int = 40,
        tile_h: int = 50,
        total_cols: int = 32,
        total_rows: int = 34
    ):
        self.extractor = extractor
        self.indexer = indexer
        self.palette_processor = palette_processor
        self.exporter = exporter
        self.tile_w = tile_w
        self.tile_h = tile_h
        self.total_cols = total_cols
        self.total_rows = total_rows

    def convert(
        self,
        image_path: str,
        output_header_path: str,
        start_col: int,
        end_col: int,
        selected_rows: List[int]
    ) -> bool:
        """
        Ejecuta el proceso completo de validación, extracción, indexación, procesamiento y exportación.
        
        Args:
            image_path (str): Ruta de la imagen origen.
            output_header_path (str): Ruta del archivo .h destino.
            start_col (int): Columna inicial de recorte.
            end_col (int): Columna final de recorte.
            selected_rows (List[int]): Lista de filas a incluir.
            
        Returns:
            bool: True si todo el proceso finaliza con éxito, False en caso de error.
        """
        start_col = max(0, min(start_col, self.total_cols - 1))
        end_col = max(start_col, min(end_col, self.total_cols - 1))

        valid_rows: List[int] = []
        for row in selected_rows:
            if 0 <= row < self.total_rows and row not in valid_rows:
                valid_rows.append(row)

        if not valid_rows:
            print("[ERROR] No hay filas válidas. Los índices permitidos van de 0 a 33.")
            return False

        try:
            print("\n[INFO] Procesando spritesheet CI8 para PS2")
            print(f"       Columnas: {start_col} a {end_col}")
            print(f"       Filas: {valid_rows}")

            # 1. Extracción y alineación con padding a múltiplos de 64
            pixels, sub_width, sub_height, num_cols, num_rows = self.extractor.extract(
                image_path, start_col, end_col, valid_rows
            )

            print(f"       Ancho con padding: {sub_width}")
            print(f"       Alto: {sub_height}")

            # 2. Indexación de colores a paleta de 256 entradas (CI8)
            palette, indices = self.indexer.index_pixels(pixels, palette_size=PS2_CLUT_PALETTE_SIZE)

            # 3. Reorganización de la paleta bajo el estándar de hardware CSM1
            palette = self.palette_processor.process(palette)

            print(f"[INFO] Entradas exportadas en la paleta: {len(palette)}")
            print(f"[INFO] Índices generados: {len(indices)}")

            # 4. Exportación optimizada en C con alineación DMA de 16 bytes
            success = self.exporter.export(
                output_path=output_header_path,
                sub_width=sub_width,
                sub_height=sub_height,
                num_cols=num_cols,
                num_rows=num_rows,
                tile_w=self.tile_w,
                tile_h=self.tile_h,
                palette=palette,
                indices=indices
            )

            if not success:
                return False

            print(f"[EXITO] Archivo generado: {output_header_path}")
            print(f"[EXITO] Tamaño de textura: {sub_width}x{sub_height}")
            print(f"[EXITO] Tamaño de índices: {len(indices)} bytes")
            print(f"[EXITO] Tamaño de paleta: {len(palette) * 4} bytes")

            return True

        except Exception as error:
            print(f"[ERROR] Ocurrió un error durante el procesamiento: {error}")
            return False


# ==============================================================================
# 4. PUNTO DE ENTRADA (CLI)
# ==============================================================================

if __name__ == "__main__":
    print("==============================================================")
    print("  Conversor de Spritesheet CI8 para PlayStation 2 (SOLID)")
    print("==============================================================")

    try:
        start_col = int(input("Columna inicial (0 a 31): "))
        end_col = int(input("Columna final (0 a 31): "))

        rows_input = input("Filas separadas por comas (ejemplo: 0, 2, 5, 12): ")
        selected_rows = [
            int(value.strip())
            for value in rows_input.split(",")
            if value.strip().isdigit()
        ]

    except ValueError:
        print("[ERROR] Formato de entrada no válido.")
        sys.exit(1)

    extractor_service = SpritesheetExtractor(tile_w=40, tile_h=50, total_cols=32, total_rows=34)
    indexer_service = CI8ColorIndexer()
    palette_service = CSM1PaletteProcessor()
    exporter_service = CHeaderCodeExporter()

    controller = SpritesheetConverterController(
        extractor=extractor_service,
        indexer=indexer_service,
        palette_processor=palette_service,
        exporter=exporter_service
    )

    success = controller.convert(
        image_path="spritesheet.png",
        output_header_path="spritesheet_ci8.h",
        start_col=start_col,
        end_col=end_col,
        selected_rows=selected_rows
    )

    if not success:
        sys.exit(1)