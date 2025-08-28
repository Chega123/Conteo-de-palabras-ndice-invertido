import os
import random

tam_gb = 2
tam_byte = tam_gb * (1024**3)

palabras = ["hola", "adios", "como", "apruebeme", "dinosaurio",
            "canse", "sanguchito", "teclado", "valles", "mundo"]

palabras_por_bloque = 4_000_000  

with open("test.txt", "w", encoding="utf-8") as f:
    escrito = 0
    while escrito < tam_byte:
        bloque = " ".join(random.choices(palabras, k=palabras_por_bloque))
        bloque += " " 
        f.write(bloque)
        escrito += len(bloque.encode("utf-8"))  

print("Archivo de 20GB generado")
