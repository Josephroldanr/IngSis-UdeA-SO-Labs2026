# Laboratorio 2: API de Procesos — Shell `wish`

En este laboratorio se implementa un intérprete de comandos (shell) simplificado llamado `wish` (Wisconsin Shell), desarrollado en lenguaje C. El objetivo principal es comprender cómo funcionan los procesos en un sistema operativo tipo UNIX: cómo se crean, ejecutan y destruyen, y cómo un shell coordina todo esto.

Se implementaron características como el modo interactivo, el modo batch, comandos integrados (`exit`, `chd`, `route`), redirección de salida estándar y ejecución de comandos en paralelo. A través de su implementación y pruebas, se refuerzan conceptos clave como `fork()`, `execv()`, `waitpid()`, `dup2()` y el manejo del search path.

## Integrantes

| NOMBRE | CORREO | DOCUMENTO |
|---|---|---|
| Joseph Roldan Ramirez | joseph.roldan@udea.edu.co | 1115091119 |
| Juan Esteban Cardozo Rivera | juan.cardozor@udea.edu.co | 1036955040 |

## Tecnologías

![C](https://img.shields.io/badge/Language-C-blue?style=flat-square&logo=c&logoColor=white) ![GCC](https://img.shields.io/badge/Compiler-GCC-green?style=flat-square&logo=gnu&logoColor=white) ![Linux](https://img.shields.io/badge/OS-Linux-yellow?style=flat-square&logo=linux&logoColor=white) ![UNIX](https://img.shields.io/badge/Paradigm-UNIX-orange?style=flat-square&logo=ubuntu&logoColor=white) ![Git](https://img.shields.io/badge/Version%20Control-Git-red?style=flat-square&logo=git&logoColor=white)

## Presentación

[![Video Explicativo](https://github.com/Josephroldanr/sistemas-operativos-labs/blob/73b8703bd617632de206f6b323444d3695c4600f/Portada.png)](https://youtu.be/XXXXXXXXXXXXXXX)

## Estructura del proyecto

```
LAB_2/
├── bin/
│   └── wish              ← ejecutable generado al compilar
├── src/
│   └── wish.c            ← código fuente del shell
├── data/
│   ├── batch.txt         ← archivo de prueba para modo batch
│   ├── batch2.txt        ← archivo de prueba para comandos paralelos
│   └── salida.txt        ← se genera al probar redirección con >
└── README.MD
```

## Compilación

Desde la raíz del proyecto `LAB_2/`:

```bash
gcc -Wall -Wextra -o bin/wish src/wish.c
```

---

## Programa `wish`

### ¿Qué hace `wish`?

`wish` es un shell simplificado inspirado en los shells de UNIX como `bash`.
Su función es **leer comandos del usuario o de un archivo**, **crear procesos hijos para ejecutarlos** y **esperar a que terminen** antes de continuar.

El programa:

* Corre en un **loop infinito** solicitando comandos al usuario
* Soporta **modo interactivo** (con prompt `wish>`) y **modo batch** (desde archivo)
* Maneja **comandos integrados** (`exit`, `chd`, `route`) sin crear procesos
* Permite **redirección de salida** con el operador `>`
* Permite **ejecución paralela** de comandos con el operador `&`
* Termina cuando el usuario escribe `exit` o al encontrar EOF

---

## Fragmentos del programa

### Inicialización del search path

```c
static void init_paths(void) {
    num_paths = 1;
    strncpy(search_paths[0], "/bin", MAX_PATH_LEN - 1);
    search_paths[0][MAX_PATH_LEN - 1] = '\0';
}
```

**Explicación:**

* Al iniciar el shell, el search path contiene únicamente `/bin`.
* El search path es el conjunto de directorios donde el shell buscará los ejecutables.
* El comando `route` permite agregar o reemplazar estos directorios durante la sesión.

---

### Búsqueda del ejecutable en el path

```c
static char *find_executable(const char *name) {
    char *full = malloc(MAX_PATH_LEN * 2);

    for (int i = 0; i < num_paths; i++) {
        snprintf(full, MAX_PATH_LEN * 2 - 1, "%s/%s",
                 search_paths[i], name);
        if (access(full, X_OK) == 0)
            return full;
    }
    free(full);
    return NULL;
}
```

**Explicación:**

* Recorre cada directorio del search path en orden.
* Para cada directorio construye la ruta completa: `directorio/nombre`.
* Usa `access(ruta, X_OK)` para verificar si el archivo existe y es ejecutable.
* Retorna la ruta completa si la encuentra, o `NULL` si no existe en ningún directorio.

---

### Parsing de un comando individual

```c
static int parse_single_command(char *token, Command *cmd) {
    cmd->argc    = 0;
    cmd->outfile = NULL;

    int redir_count = 0;
    for (char *p = token; *p; p++) {
        if (*p == '>') redir_count++;
    }

    if (redir_count > 1) {
        print_error();
        return -1;
    }
    // ... separación por '>' y luego por espacios
}
```

**Explicación:**

* Primero detecta cuántos operadores `>` hay en el token.
* Si hay más de uno, es un error (no se permite redirección múltiple).
* Si hay exactamente uno, separa el comando del nombre del archivo de salida.
* Luego divide el lado izquierdo por espacios para construir el arreglo `argv[]`.

---

### Ejecución de un comando externo con `fork()` y `execv()`

```c
pid_t pid = fork();
if (pid == 0) {
    // Proceso hijo
    if (cmd->outfile) {
        int fd = open(cmd->outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);
    }
    execv(exec_path, cmd->argv);
    print_error();
    exit(1);
} else {
    // Proceso padre: retorna el pid para waitpid() posterior
    return pid;
}
```

**Explicación:**

* `fork()` crea una copia exacta del proceso actual (proceso hijo).
* En el hijo, si hay redirección, se abre el archivo y se usan dos `dup2()` para redirigir tanto `stdout` como `stderr` a ese archivo.
* Luego `execv()` reemplaza la imagen del proceso hijo con el ejecutable real.
* Si `execv()` retorna, significa que hubo un error (el comando no pudo ejecutarse).
* El proceso padre recibe el PID del hijo y lo usa más adelante para `waitpid()`.

---

### Comandos paralelos con `&`

```c
static void process_line(char *line) {
    // Dividir por '&'
    while ((tok = strsep(&rest, "&")) != NULL) {
        cmd_tokens[num_cmds++] = tok;
    }

    // Lanzar todos los comandos
    for (int i = 0; i < num_cmds; i++) {
        pid_t pid = execute_command(&cmd);
        if (pid > 0) pids[pid_count++] = pid;
    }

    // Esperar a que todos terminen
    for (int i = 0; i < pid_count; i++) {
        waitpid(pids[i], NULL, 0);
    }
}
```

**Explicación:**

* La línea de entrada se divide por `&` para obtener los comandos individuales.
* Todos los comandos se lanzan con `fork()` antes de esperar a cualquiera.
* Una vez lanzados todos, se hace `waitpid()` por cada PID acumulado.
* Esto garantiza que los comandos corran verdaderamente en paralelo.

---

### Comando integrado `chd`

```c
static void builtin_chd(Command *cmd) {
    if (cmd->argc != 2) {
        print_error();
        return;
    }
    if (chdir(cmd->argv[1]) != 0)
        print_error();
}
```

**Explicación:**

* Verifica que se haya pasado exactamente un argumento (el directorio destino).
* Usa `chdir()` directamente en el proceso del shell, sin `fork()`.
* Esto es fundamental: si se usara `fork()`, el cambio afectaría solo al hijo y el shell padre permanecería en el mismo directorio.

---

### Comando integrado `route`

```c
static void builtin_route(Command *cmd) {
    num_paths = 0;
    for (int i = 1; i < cmd->argc && num_paths < MAX_PATHS; i++) {
        strncpy(search_paths[num_paths], cmd->argv[i], MAX_PATH_LEN - 1);
        num_paths++;
    }
}
```

**Explicación:**

* Primero pone `num_paths = 0`, borrando completamente el path anterior.
* Luego agrega cada argumento recibido como un nuevo directorio del path.
* Si se llama sin argumentos, el path queda vacío y el shell ya no puede ejecutar ningún comando externo (solo los built-ins siguen disponibles).

---

### Loop principal del shell

```c
static void run_shell(FILE *input, int interactive) {
    char   *line = NULL;
    size_t  len  = 0;

    while (1) {
        if (interactive) {
            printf("wish> ");
            fflush(stdout);
        }

        if (getline(&line, &len, input) == -1) {
            free(line);
            exit(0);
        }

        process_line(line);
    }
}
```

**Explicación:**

* Si el modo es interactivo, imprime el prompt `wish> ` antes de cada lectura.
* Usa `getline()` para leer líneas de longitud arbitraria de forma segura.
* Si `getline()` retorna `-1` (EOF), el shell termina llamando a `exit(0)`.
* Cada línea leída se pasa a `process_line()` para ser procesada.

---

## Ejecución del programa

### Caso 1: Modo interactivo — comando simple

```bash
./bin/wish
wish> ls
```

#### Salida esperada:

```text
bin  data  src  README.MD
```

El shell imprime el prompt `wish>`, lee el comando, lo ejecuta y vuelve a mostrar el prompt.

![wish](https://github.com/Josephroldanr/IngSis-UdeA-SO-Labs2026/blob/main/LAB_2/data/wish_caso1.gif)

---

### Caso 2: Modo batch

#### Contenido de `data/batch.txt`:

```text
ls /tmp
ls /home
```

```bash
./bin/wish data/batch.txt
```

#### Salida esperada:

```text
(contenido de /tmp)
(contenido de /home)
```

En modo batch el prompt **no se imprime**. Los comandos se ejecutan en orden y el shell termina al llegar al final del archivo.

![wish](https://github.com/Josephroldanr/IngSis-UdeA-SO-Labs2026/blob/main/LAB_2/data/wish_caso2.gif)

---

### Caso 3: Built-in `chd`

```bash
./bin/wish
wish> chd /tmp
wish> ls
```

#### Salida esperada:

```text
(contenido del directorio /tmp)
```

El shell cambia al directorio `/tmp` y el siguiente `ls` muestra su contenido.

![wish](https://github.com/Josephroldanr/IngSis-UdeA-SO-Labs2026/blob/main/LAB_2/data/wish_caso3.gif)

---

### Caso 4: Built-in `route`

```bash
./bin/wish
wish> route /bin /usr/bin
wish> ls
```

#### Salida esperada:

```text
bin  data  src  README.MD
```

El shell ahora busca ejecutables tanto en `/bin` como en `/usr/bin`.

![wish](https://github.com/Josephroldanr/IngSis-UdeA-SO-Labs2026/blob/main/LAB_2/data/wish_caso4.gif)

---

### Caso 5: `route` vacío — bloquea comandos externos

```bash
./bin/wish
wish> route
wish> ls
An error has occurred
```

Con el path vacío el shell no puede encontrar ningún ejecutable externo. Solo los built-ins (`exit`, `chd`, `route`) siguen funcionando.

![wish](https://github.com/Josephroldanr/IngSis-UdeA-SO-Labs2026/blob/main/LAB_2/data/wish_caso5.gif)

---

### Caso 6: Redirección de salida con `>`

```bash
./bin/wish
wish> ls > data/salida.txt
```

#### Salida en pantalla:

```text
(no se imprime nada)
```

#### Contenido de `data/salida.txt`:

```text
bin  data  src  README.MD
```

Tanto `stdout` como `stderr` son redirigidos al archivo. Si el archivo ya existe, se sobrescribe.

![wish](https://github.com/Josephroldanr/IngSis-UdeA-SO-Labs2026/blob/main/LAB_2/data/wish_caso6.gif)

---

### Caso 7: Redirección doble (error)

```bash
./bin/wish
wish> ls > data/a.txt > data/b.txt
An error has occurred
```

Más de un operador `>` en el mismo comando es un error. El shell imprime el mensaje y continúa.

![wish](https://github.com/Josephroldanr/IngSis-UdeA-SO-Labs2026/blob/main/LAB_2/data/wish_caso7.gif)

---

### Caso 8: Comandos paralelos con `&`

#### Contenido de `data/batch2.txt`:

```text
ls /tmp & echo hola & pwd
```

```bash
./bin/wish data/batch2.txt
```

#### Salida esperada (orden puede variar):

```text
hola
/ruta/actual
(contenido de /tmp)
```

Los tres comandos se lanzan en paralelo antes de esperar a que cualquiera termine. El orden de salida puede variar entre ejecuciones.

![wish](https://github.com/Josephroldanr/IngSis-UdeA-SO-Labs2026/blob/main/LAB_2/data/wish_caso8.gif)

---

### Caso 9: `exit` con argumentos (error)

```bash
./bin/wish
wish> exit argumento
An error has occurred
wish>
```

El built-in `exit` no acepta argumentos. Si se pasan, se imprime el error y el shell continúa funcionando.

![wish](https://github.com/Josephroldanr/IngSis-UdeA-SO-Labs2026/blob/main/LAB_2/data/wish_caso9.gif)

---

### Caso 10: Comando inexistente

```bash
./bin/wish
wish> comandofalso
An error has occurred
wish>
```

Si el ejecutable no se encuentra en ningún directorio del path, se imprime el mensaje de error y el shell continúa.

![wish](https://github.com/Josephroldanr/IngSis-UdeA-SO-Labs2026/blob/main/LAB_2/data/wish_caso10.gif)

---

### Caso 11: Más de un argumento al invocar wish (error)

```bash
./bin/wish data/batch.txt data/batch2.txt
An error has occurred
$ echo $?
1
```

El shell solo acepta cero o un argumento. Con más de uno imprime el error y termina con código de salida `1`.

![wish](https://github.com/Josephroldanr/IngSis-UdeA-SO-Labs2026/blob/main/LAB_2/data/wish_caso11.gif)

---

### Caso 12: EOF en modo interactivo

```bash
./bin/wish
wish> (Ctrl+D)
$ echo $?
0
```

Al presionar `Ctrl+D` se envía EOF. El shell llama a `exit(0)` y termina limpiamente.

![wish](https://github.com/Josephroldanr/IngSis-UdeA-SO-Labs2026/blob/main/LAB_2/data/wish_caso12.gif)

---

## Problemas presentados durante el desarrollo de la práctica y sus soluciones

Durante el desarrollo se presentaron varios inconvenientes técnicos y conceptuales que fueron resueltos a medida que se avanzaba en la implementación y las pruebas del shell.

### 1. `chd` no cambiaba el directorio del shell

**Problema:**
La primera versión de `chd` usaba `fork()` + `chdir()`, igual que los comandos externos. El directorio cambiaba, pero solo en el proceso hijo. El shell padre permanecía en el mismo directorio.

**Cómo nos dimos cuenta:**
Al ejecutar `chd /tmp` seguido de `ls`, el contenido mostrado no correspondía a `/tmp` sino al directorio original del shell.

**Solución:**
Se implementó `chd` como un built-in que llama a `chdir()` directamente en el proceso del shell, sin usar `fork()`. Así el cambio afecta al proceso principal y todos los comandos siguientes lo ven.

---

### 2. Espacios múltiples rompían el parsing

**Problema:**
Comandos como `ls   -la   /tmp` (con varios espacios entre argumentos) generaban tokens vacíos que se agregaban al arreglo `argv[]`, causando fallos en `execv()`.

**Cómo nos dimos cuenta:**
Al probar con espacios extras, el comando fallaba con "An error has occurred" aunque el ejecutable existía.

**Solución:**
Se agregó una verificación al construir `argv[]`: si el token devuelto por `strsep()` es una cadena vacía (`*word == '\0'`), se descarta. Así se ignoran los delimitadores consecutivos.

---

### 3. `stderr` no se redirigía al archivo con `>`

**Problema:**
Al redirigir la salida con `>`, los mensajes de error del programa ejecutado seguían apareciendo en la terminal en lugar de ir al archivo.

**Cómo nos dimos cuenta:**
Al ejecutar `ls /directorio_inexistente > data/salida.txt`, el mensaje de error de `ls` aparecía en pantalla en vez de quedar en `salida.txt`.

**Solución:**
Se añadió un segundo `dup2(fd, STDERR_FILENO)` después del `dup2(fd, STDOUT_FILENO)`. Esto redirige tanto la salida estándar como la salida de error al mismo archivo, que es el comportamiento especificado en el enunciado.

---

### 4. `route` vacío no bloqueaba los comandos externos

**Problema:**
Al ejecutar `route` sin argumentos, se esperaba que el shell dejara de poder ejecutar comandos externos, pero el path anterior permanecía.

**Cómo nos dimos cuenta:**
Tras `route` sin argumentos, `ls` seguía funcionando normalmente.

**Solución:**
Se agregó `num_paths = 0` al inicio de `builtin_route()`, antes de agregar los nuevos directorios. Si no hay argumentos, `num_paths` queda en `0` y `find_executable()` no itera sobre ningún directorio, retornando `NULL` siempre.

---

### 5. El prompt aparecía en modo batch

**Problema:**
En las primeras versiones, el prompt `wish>` se imprimía también en modo batch, mezclándose con la salida de los comandos.

**Cómo nos dimos cuenta:**
Al ejecutar `./bin/wish data/batch.txt`, la salida incluía líneas con `wish>` que no deberían estar.

**Solución:**
Se pasó un parámetro `interactive` a `run_shell()`. El prompt solo se imprime si `interactive == 1`. En modo batch se pasa `0` y el prompt nunca aparece.

---

### 6. Verificación del código de salida

**Problema:**
Inicialmente no era claro cómo verificar que el shell terminaba con el código correcto (`0` o `1`) en cada situación.

**Cómo nos dimos cuenta:**
No se mostraba ningún valor numérico en la consola al terminar el programa.

**Solución:**
Se aprendió a usar `echo $?` inmediatamente después de ejecutar el shell para ver el código de salida del último proceso. Se incorporó este procedimiento en todas las pruebas de casos de error.

---

## Manifiesto de transparencia: uso de IA generativa

Durante el desarrollo de esta práctica se utilizó una herramienta de IA generativa (Claude de Anthropic) como apoyo puntual en el proceso de aprendizaje y desarrollo.

La IA se empleó principalmente para:

* Aclarar el comportamiento de las syscalls `fork()`, `execv()`, `dup2()`, `waitpid()` y `chdir()` antes de implementarlas.
* Generar el código fuente de `wish.c` siguiendo estrictamente las especificaciones del enunciado, el cual fue revisado, compilado y probado manualmente por el equipo.
* Revisar y mejorar la redacción de este `README.MD`, adaptándolo al formato del laboratorio anterior.
* Identificar errores conceptuales en la implementación y proponer soluciones coherentes con el enunciado.

La comprensión del código, las pruebas realizadas y las decisiones de diseño fueron validadas por los integrantes del grupo. La IA se utilizó como herramienta de consulta, similar a la documentación técnica o foros especializados, y no como reemplazo del proceso de aprendizaje.
