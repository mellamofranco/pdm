import re
import sys
import os

def remove_comments_and_empty_lines(input_file, output_file=None):
    """
    Elimina comentarios y líneas vacías de un archivo de Arduino.
    - Elimina comentarios de una línea (// comentario)
    - Elimina comentarios de múltiples líneas (/* comentario */)
    - Elimina líneas vacías o que solo contienen espacios en blanco
    
    Args:
        input_file (str): Ruta al archivo de entrada
        output_file (str): Ruta al archivo de salida. Si es None, sobreescribirá el archivo original.
    """
    # Si no se especifica un archivo de salida, usamos un archivo temporal
    if output_file is None:
        output_file = input_file + ".temp"
        overwrite_original = True
    else:
        overwrite_original = False
        
    try:
        # Leer el contenido del archivo
        with open(input_file, 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Eliminar comentarios de múltiples líneas (/* ... */)
        content = re.sub(r'/\*[\s\S]*?\*/', '', content)
        
        # Eliminar comentarios de una línea (// ...)
        content = re.sub(r'//.*', '', content)
        
        # Dividir en líneas para procesar líneas vacías
        lines = content.split('\n')
        
        # Filtrar líneas vacías o que solo contienen espacios en blanco
        non_empty_lines = [line for line in lines if line.strip()]
        
        # Unir las líneas no vacías nuevamente
        clean_content = '\n'.join(non_empty_lines)
        
        # Escribir el contenido limpio al archivo de salida
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(clean_content)
            
        # Si se debe sobreescribir el archivo original
        if overwrite_original:
            os.replace(output_file, input_file)
            print(f"Se han eliminado los comentarios y líneas vacías de {input_file}")
        else:
            print(f"Se han eliminado los comentarios y líneas vacías. Resultado guardado en {output_file}")
            
    except Exception as e:
        print(f"Error: {e}")
        
if __name__ == "__main__":
    # Si se ejecuta directamente, comprueba los argumentos de línea de comandos
    if len(sys.argv) < 2:
        print("Uso: python remove_arduino_comments.py archivo_entrada.ino [archivo_salida.ino]")
    elif len(sys.argv) == 2:
        remove_comments_and_empty_lines(sys.argv[1])
    else:
        remove_comments_and_empty_lines(sys.argv[1], sys.argv[2])