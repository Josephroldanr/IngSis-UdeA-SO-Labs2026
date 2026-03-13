# Laboratorio 1: Comando UNIX en C

En este laboratorio se implementan versiones simplificadas de algunas herramientas básicas de UNIX utilizando el lenguaje C. El objetivo principal es comprender el manejo de archivos, argumentos de línea de comandos y la entrada y salida estándar en un sistema tipo UNIX.

Se desarrollaron los programas wcat, wgrep, wzip y wunzip, los cuales permiten mostrar archivos de texto, buscar patrones, comprimir información y descomprimirla. A través de su implementación y pruebas, se refuerzan conceptos fundamentales como el uso de redirecciones, el manejo de errores y la diferencia entre archivos de texto y binarios.

## Integrantes


| NOMBRE      | CORRREO                | DOCUMENTO |
|-----------------------|-----------------------------------|-----------------|
| Joseph Roldan Ramirez    | joseph.roldan@udea.edu.co        |  1115091119     |
| Juan Esteban Cardonzo Rivera   | juan.cardozor@udea.edu.co       |  1036955040     |

## Tecnologias

![C](https://img.shields.io/badge/Language-C-blue?style=flat-square&logo=c&logoColor=white) ![GCC](https://img.shields.io/badge/Compiler-GCC-green?style=flat-square&logo=gnu&logoColor=white) ![Linux](https://img.shields.io/badge/OS-Linux-yellow?style=flat-square&logo=linux&logoColor=white) ![UNIX](https://img.shields.io/badge/Paradigm-UNIX-orange?style=flat-square&logo=ubuntu&logoColor=white) ![Git](https://img.shields.io/badge/Version%20Control-Git-red?style=flat-square&logo=git&logoColor=white)

## Presentacion

[Video Explicativo del Laboratorio]()

## Programa `wcat`


### ¿Qué hace `wcat`?

`wcat` es una versión simplificada del comando `cat` de UNIX.
Su función es **leer uno o más archivos de texto** recibidos por línea de comandos y **mostrar su contenido completo en la salida estándar**, sin modificarlo.

El programa:

* Lee archivos **línea por línea**
* Imprime el contenido en el orden recibido
* Finaliza si ocurre un error al abrir un archivo


## Fragmentos del programa

**Apertura del archivo**

```c
FILE *openFile(char *ruta) {
    FILE *file = fopen(ruta, "r");

    if (file == NULL) {
        puts("wcat: cannot open file");
        exit(1);
    }

    return file;
}
```

**Explicación:**

* Se intenta abrir el archivo en modo lectura (`"r"`).
* Si `fopen` retorna `NULL`, significa que el archivo no existe.
* En ese caso, se imprime el mensaje de error exacto solicitado y el programa termina con código de estado `1`.
* Si la apertura es exitosa, se retorna el puntero al archivo.


**Lectura e impresión del archivo**

```c
void printFile(FILE *file) {
    char buffer[1024];

    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        printf("%s", buffer);
    }

    fclose(file);
}
```

**Explicación:**

* Se utiliza un buffer de tamaño fijo para leer el archivo línea por línea.
* `fgets` permite leer hasta que se alcance el final del archivo.
* Cada línea leída se imprime directamente en la salida estándar.
* Al finalizar, el archivo se cierra correctamente con `fclose`.

**Función principal**

```c
int main(int argc, char *argv[]) {
    if (argc <= 1) {
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        FILE *nowFile = openFile(argv[i]);
        printFile(nowFile);
    }

    return 0;
}
```

**Explicación:**

* Verifica que el usuario haya proporcionado al menos un archivo.
* Recorre todos los archivos recibidos como argumentos.
* Para cada archivo:
  * Lo abre usando `openFile`.
  * Imprime su contenido usando `printFile`.


## Ejecución del programa

#### Caso 1: Mostrar un archivo

```bash
./bin/wcat data/archivo1.txt
```

#### Salida esperada:

```text
Hola mundo
Este es el archivo uno
Fin del archivo
```
![wcat](https://github.com/Josephroldanr/sistemas-operativos-labs/blob/95c7b563773ef3f243c0b7bee7ac3085ede61506/wcat%20caso1.gif)

#### Caso 2: Mostrar múltiples archivos

```bash
./bin/wcat data/archivo1.txt data/archivo2.txt
```

#### Salida esperada:

```text
Hola mundo
Este es el archivo uno
Fin del archivo

Segundo archivo
Otra línea
```
![wcat](https://github.com/Josephroldanr/sistemas-operativos-labs/blob/95c7b563773ef3f243c0b7bee7ac3085ede61506/wcat%20caso2.gif)

#### Caso 3: Archivo vacio

```bash
./bin/wcat data/vacio.txt
```

#### Salida obtenida:

```text
(no se imprime nada)
```

Esto **NO es un error**.
Es el comportamiento correcto. Y el programa termina inmediatamente con **código de salida 0**, cumpliendo el enunciado del taller.


![wcat](https://github.com/Josephroldanr/sistemas-operativos-labs/blob/95c7b563773ef3f243c0b7bee7ac3085ede61506/wcat%20caso3.gif)


#### Caso 4: Archivo inexistente


```bash
./bin/wcat data/archivo5.txt
```

Si el programa no puede abrir un archivo, se imprime el siguiente mensaje:

#### Salida obtenida:

```text
wcat: cannot open file
```

Y el programa termina inmediatamente con **código de salida 1**, cumpliendo el enunciado del taller.

![wcat](https://github.com/Josephroldanr/sistemas-operativos-labs/blob/ff845eee6752c0fcecb2d8316757b33a35277b42/wcat%20caso4.gif)


## Programa `wgrep`

### ¿Qué hace `wgrep`?

`wgrep` es una versión simplificada del comando `grep` de UNIX.
Su función es **buscar un patrón de texto dentro de un archivo** y **mostrar únicamente las líneas que contienen dicho patrón**.

El programa:

* Recibe un **patrón** y un **archivo**
* Lee el archivo línea por línea
* Imprime solo las líneas donde aparece el patrón
* No imprime nada si no hay coincidencias

## Fragmentos del programa

**Lectura de argumentos y validación**

```c
if (argc < 3) {
    return 0;
}
```

**Explicación:**

* El programa necesita al menos:
  * un patrón (`argv[1]`)
  * un archivo (`argv[2]`)
* Si no se proporcionan los argumentos necesarios, el programa termina sin mostrar salida.
* Este comportamiento es consistente con herramientas UNIX simples.

**Búsqueda del patrón en cada línea**

```c
if (strstr(buffer, pattern) != NULL) {
    printf("%s", buffer);
}
```

**Explicación:**

* `strstr` busca si el patrón aparece dentro de la línea leída.
* Si retorna un valor distinto de `NULL`, significa que hay coincidencia.
* Solo en ese caso la línea se imprime.
* La búsqueda es **sensible a mayúsculas y minúsculas**.

**Lectura del archivo línea por línea**

```c
while (fgets(buffer, sizeof(buffer), file) != NULL) {
    if (strstr(buffer, pattern) != NULL) {
        printf("%s", buffer);
    }
}
```

**Explicación:**

* El archivo se recorre línea por línea usando `fgets`.
* Cada línea se analiza de forma independiente.
* Esto evita cargar todo el archivo en memoria.

## Ejecución del programa

#### Caso 1: Patrón encontrado

```bash
./bin/wgrep hola data/texto.txt
```

#### Contenido del archivo:

```text
hola mundo
esto es una prueba
aquí aparece foo
FOO no es lo mismo
barfood también cuenta
fin del archivo
```

#### Salida esperada:

```text
hola mundo
hola otra vez
```

![wgrep](https://github.com/Josephroldanr/sistemas-operativos-labs/blob/30b02f53eb53b9b7559a5d585cd202ccf205b4cc/wgrep%20caso1.gif)

---

#### Caso 2: Patrón no encontrado

```bash
./bin/wgrep adios data/texto.txt
```

#### Salida esperada:

```text
(no se imprime nada)
```

Esto **NO es un error**.
Es el comportamiento correcto. Y el programa termina inmediatamente con **código de salida 0**, cumpliendo el enunciado del taller.

![wgrep](https://github.com/Josephroldanr/sistemas-operativos-labs/blob/30b02f53eb53b9b7559a5d585cd202ccf205b4cc/wgrep%20caso2.gif)

---

#### Caso 3: Archivo inexistente

```bash
./bin/wgrep hola data/no_existe.txt
```

#### Salida obtenida:

Si el archivo no puede abrirse, el programa imprime:

```text
wgrep: cannot open file
```

Y termina inmediatamente con **código de salida 1**.

![wgrep](https://github.com/Josephroldanr/sistemas-operativos-labs/blob/30b02f53eb53b9b7559a5d585cd202ccf205b4cc/wgrep%20caso3.gif)


## Programa `wzip`

### ¿Qué hace `wzip`?

`wzip` es una versión simplificada de una herramienta de **compresión**.
Implementa el algoritmo **Run-Length Encoding (RLE)**, el cual **comprime texto contando caracteres repetidos consecutivos**.

Ejemplo:

Texto original:

```
aaaaaaaaaabbbb
```

`wzip` lo comprime en binario como algo parecido a:

```
[4][a][4][b]
```
IMPORTANTE: La salida de `wzip` **NO es texto**, es **binaria**, por eso normalmente se redirige a un archivo `.z`.

## Fragmentos del programa

**Lectura de archivos carácter por carácter**

```c
fread(&curr, sizeof(char), 1, file);
```

**Explicación:**

* `fread` permite leer datos binarios.
* Se lee **un carácter a la vez** para poder contar repeticiones.
* Esto es necesario porque `wzip` trabaja a nivel de caracteres, no de líneas.

**Conteo de caracteres repetidos**

```c
if (curr == prev) {
    count++;
} else {
    fwrite(&count, sizeof(int), 1, stdout);
    fwrite(&prev, sizeof(char), 1, stdout);
    prev = curr;
    count = 1;
}
```

**Explicación:**

* Se compara el carácter actual con el anterior.
* Si son iguales, se incrementa el contador.
* Si son distintos:
  * Se escribe el número de repeticiones (`int`)
  * Se escribe el carácter correspondiente (`char`)
  * Se reinicia el conteo


** Escritura de datos binarios**

```c
fwrite(&count, sizeof(int), 1, stdout);
fwrite(&prev, sizeof(char), 1, stdout);
```

**Explicación:**

* Los datos se escriben en formato binario.
* Primero el contador (`int`)
* Luego el carácter (`char`)
* La salida se envía a `stdout` para permitir redirección con `>` o `|`.


## Ejecución del programa

#### Caso 1: Comprimir un archivo

Tu archivo original:
```bash
aaaaaaaaaabbbb
```

```bash
./bin/wzip data/archivo.txt > data/zip1.z
```
No se muestra nada en pantalla porque la salida es binaria.

![wzip](https://github.com/Josephroldanr/sistemas-operativos-labs/blob/df3b0e1dfba24053e0dbcfccad14809bfd9faa74/wzip%20caso1.gif)



#### Caso 2: Visualización del archivo comprimido

```bash
cat data/zip1.z
```

#### Salida esperada:

```
ab
```
Nota: Aunque el archivo es binario, algunos caracteres pueden mostrarse de forma legible al usar `cat`. Esto no significa que el archivo sea texto, sino que cat imprime los bytes visibles y omite los bytes nulos (\0).

![wzip](https://github.com/Josephroldanr/sistemas-operativos-labs/blob/df3b0e1dfba24053e0dbcfccad14809bfd9faa74/wzip%20caso2.gif)

## Programa `wunzip`

### ¿Qué hace `wunzip`?

`wunzip` es el programa inverso de `wzip`.
Su función es **descomprimir archivos binarios generados por `wzip`** y **reconstruir el texto original**, imprimiéndolo en la salida estándar.

El programa:

* Lee datos binarios en el formato generado por `wzip`
* Interpreta pares `(contador, carácter)`
* Imprime el carácter repetido el número de veces indicado
* Produce **texto plano** como salida

---

## Fragmentos claves del programa (Código y explicación)

**Lectura de datos binarios**

```c
fread(&count, sizeof(int), 1, file);
fread(&ch, sizeof(char), 1, file);
```

**Explicación:**

* Se leen los datos en el **mismo orden** en que fueron escritos por `wzip`.
* Primero se lee un entero (`count`) que indica cuántas veces se repite el carácter.
* Luego se lee el carácter (`ch`).
* Esta lectura se realiza desde un archivo binario.

---

**Reconstrucción del texto original**

```c
for (int i = 0; i < count; i++) {
    printf("%c", ch);
}
```

**Explicación:**

* Se imprime el carácter leído tantas veces como indique el contador.
* De esta forma se reconstruye exactamente el texto original antes de la compresión.
* La salida es **texto plano**, por lo que puede mostrarse directamente en la terminal.

---

**Bucle principal de descompresión**

```c
while (fread(&count, sizeof(int), 1, file) == 1) {
    fread(&ch, sizeof(char), 1, file);
    for (int i = 0; i < count; i++) {
        printf("%c", ch);
    }
}
```

**Explicación:**

* El ciclo continúa mientras se puedan leer datos válidos del archivo.
* Cada iteración procesa un par `(contador, carácter)`.
* El ciclo termina al llegar al final del archivo.



## Ejecución del programa 

#### Caso 1: Descomprimir un archivo `.z`

```bash
./bin/wunzip data/zip1.z
```

**Salida esperada:**

```text
aaaaaaaaaabbbb
```

![wunzip](https://github.com/Josephroldanr/sistemas-operativos-labs/blob/812bdf67a250e940b2f3dad82293cef6f95c37e6/wunzip%20caso1.gif)

#### Caso 2: Prueba completa `wzip → wunzip`

```bash
./bin/wzip data/zip2.txt > data/zip2.z
./bin/wunzip data/zip2.z
```

**Salida esperada:**

```text
ccccccccccddddddaaaa
```

Si esta salida coincide exactamente con el archivo original,
entonces **tanto `wzip` como `wunzip` funcionan correctamente**.

![wunzip](https://github.com/Josephroldanr/sistemas-operativos-labs/blob/812bdf67a250e940b2f3dad82293cef6f95c37e6/wunzip%20caso2.gif)


## Problemas presentados durante el desarrollo de la práctica y sus soluciones

Durante el desarrollo de la práctica se presentaron varios inconvenientes, tanto técnicos como conceptuales, que fueron resueltos a medida que se avanzaba en la implementación y las pruebas de los programas.


### 1. Mensajes de error que no aparecían en la consola

**Problema:**
En el programa `wzip`, al intentar abrir un archivo inexistente, el mensaje
`wzip: cannot open file` no siempre aparecía en la consola, especialmente cuando se utilizaba redirección de salida (`>`).

**Cómo nos dimos cuenta:**
Al ejecutar el programa y redirigir la salida a un archivo, parecía que el error no se estaba mostrando, lo que inicialmente se interpretó como un fallo en el manejo de errores.

**Solución:**
Se investigó el comportamiento de la redirección en sistemas UNIX y se entendió que `printf` escribe en la salida estándar (`stdout`), la cual estaba siendo redirigida a un archivo.
Se comprobó que el mensaje sí se generaba correctamente y se documentó este comportamiento en el informe.


### 2. Confusión al visualizar archivos comprimidos

**Problema:**
Se esperaba que al usar `cat` sobre un archivo comprimido (`.z`) se mostraran caracteres completamente ilegibles.

**Cómo nos dimos cuenta:**
Al probar con archivos simples, como aquellos que contenían repeticiones de letras, `cat` mostraba caracteres legibles como `ab`, lo que generó dudas sobre si la compresión estaba funcionando correctamente.

**Solución:**
Se comprendió que los archivos comprimidos contienen datos binarios y que `cat` solo muestra los bytes visibles, omitiendo los bytes nulos.


### 3. Uso de pipes entre `wzip` y `wunzip`

**Problema:**
Se intentó ejecutar `wzip` y `wunzip` utilizando una tubería (`|`), pero `wunzip` mostraba el mensaje de uso incorrecto.

**Cómo nos dimos cuenta:**
Al ejecutar el comando, `wunzip` indicaba que esperaba archivos como argumentos, lo que impedía el uso directo de pipes.

**Solución:**
Se revisó la implementación y se confirmó que `wunzip` fue diseñado para recibir archivos físicos y no para leer desde la entrada estándar. Se ajustaron las pruebas y se documentó esta limitación como una decisión de diseño coherente con el enunciado del laboratorio.

### 4. Manejo de archivos vacíos

**Problema:**
No estaba claro cómo debían comportarse los programas al recibir archivos vacíos.

**Cómo nos dimos cuenta:**
Al ejecutar los programas con archivos sin contenido, no se producía ninguna salida ni error, lo que generó dudas sobre si este comportamiento era correcto.

**Solución:**
Se verificó que este comportamiento es válido y esperado en sistemas UNIX. Se documentó que un archivo vacío no produce salida y no se considera un error en ninguno de los programas desarrollados.


### 5. Errores por rutas incorrectas y estructura del proyecto

**Problema:**
En varias ocasiones los programas no encontraban los archivos de entrada.

**Cómo nos dimos cuenta:**
Los errores aparecían cuando se ejecutaban los programas desde distintos directorios o cuando se usaban rutas incompletas.

**Solución:**
Se reorganizó el proyecto en carpetas claras (`src`, `bin`, `data`) y se estandarizó el uso de rutas relativas correctas en todos los comandos de prueba.


### 6. Verificación del código de salida

**Problema:**
Inicialmente se esperaba que el código de salida (`0` o `1`) se mostrara directamente en la consola.

**Cómo nos dimos cuenta:**
Al ejecutar los programas, no se mostraba ningún valor numérico tras finalizar la ejecución.

**Solución:**
Se aprendió a verificar el código de salida usando `echo $?` y se incorporó este procedimiento en las pruebas para validar los casos de error.


## Manifiesto de transparencia: uso de IA generativa

Durante el desarrollo de esta práctica se utilizó una herramienta de IA generativa como apoyo puntual en el proceso de aprendizaje.

La IA se empleó principalmente para:

* Aclarar conceptos sobre el funcionamiento de sistemas UNIX (redirecciones, entrada y salida estándar).
* Revisar y mejorar la redacción del archivo `README.md`.
* Identificar errores conceptuales en la documentación y ajustarlos a comportamientos reales del sistema.

La implementación del código, las pruebas realizadas y las decisiones de diseño fueron desarrolladas por los integrantes del grupo. La IA no generó automáticamente las soluciones completas, sino que se utilizó como una herramienta de consulta similar a la documentación técnica o foros especializados.

