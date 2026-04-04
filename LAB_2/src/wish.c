#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

/* Mensaje de error único */
#define ERROR_MSG "An error has occurred\n"

static void print_error(void) {
    write(STDERR_FILENO, ERROR_MSG, strlen(ERROR_MSG));
}

#define MAX_PATHS 64
#define MAX_PATH_LEN 256

static char search_paths[MAX_PATHS][MAX_PATH_LEN];
static int  num_paths = 0;

/* Inicializa el path con /bin */
static void init_paths(void) {
    num_paths = 1;
    strncpy(search_paths[0], "/bin", MAX_PATH_LEN - 1);
    search_paths[0][MAX_PATH_LEN - 1] = '\0';
}

#define MAX_ARGS 128

typedef struct {
    char *argv[MAX_ARGS]; 
    int   argc;
    char *outfile;
} Command;


static char *find_executable(const char *name) {
    char *full = malloc(MAX_PATH_LEN * 2);
    if (!full) return NULL;

    for (int i = 0; i < num_paths; i++) {
        snprintf(full, MAX_PATH_LEN * 2 - 1, "%s/%s",
                 search_paths[i], name);
        if (access(full, X_OK) == 0)
            return full;
    }
    free(full);
    return NULL;
}

/* 
   Built-in: exit 
   No admite argumentos
 */
static void builtin_exit(Command *cmd) {
    if (cmd->argc != 1) {   /* exit no acepta argumentos */
        print_error();
        return;
    }
    exit(0);
}

/* 
   Built-in: chd 
   Exactamente un argumento
*/
static void builtin_chd(Command *cmd) {
    if (cmd->argc != 2) {
        print_error();
        return;
    }
    if (chdir(cmd->argv[1]) != 0)
        print_error();
}


static void builtin_route(Command *cmd) {
    num_paths = 0;
    for (int i = 1; i < cmd->argc && num_paths < MAX_PATHS; i++) {
        strncpy(search_paths[num_paths], cmd->argv[i], MAX_PATH_LEN - 1);
        search_paths[num_paths][MAX_PATH_LEN - 1] = '\0';
        num_paths++;
    }
}


static char *trim(char *str) {
    while (*str == ' ' || *str == '\t') str++;
    char *end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r'))
        *end-- = '\0';
    return str;
}


static int parse_single_command(char *token, Command *cmd) {
    cmd->argc    = 0;
    cmd->outfile = NULL;

    /* Buscamos el operador de redirección '>' */
    int redir_count = 0;
    char *redir_pos = NULL;
    for (char *p = token; *p; p++) {
        if (*p == '>') {
            redir_count++;
            redir_pos = p;
        }
    }

    /* Más de un '>' es error (sección 2.5) */
    if (redir_count > 1) {
        print_error();
        return -1;
    }

    char *cmd_part   = token;
    char *file_part  = NULL;

    if (redir_count == 1) {
        *redir_pos = '\0'; 
        file_part  = redir_pos + 1; 
        file_part  = trim(file_part);

        /* Archivo vacío o múltiples tokens a la derecha son error */
        if (strlen(file_part) == 0) {
            print_error();
            return -1;
        }
        /* Verificamos que no haya espacios internos (sería más de un archivo) */
        int tokens = 0;
        char *copy = strdup(file_part);
        char *rest = copy;
        char *word;
        while ((word = strsep(&rest, " \t")) != NULL) {
            if (*word != '\0') tokens++;
        }
        free(copy);
        if (tokens != 1) {
            print_error();
            return -1;
        }
        cmd->outfile = file_part;
    }

    /* Parsear el lado izquierdo: nombre del comando y sus argumentos */
    char *rest = cmd_part;
    char *word;
    while ((word = strsep(&rest, " \t")) != NULL) {
        if (*word == '\0') continue;  /* ignorar espacios múltiples */
        if (cmd->argc >= MAX_ARGS - 1) break;
        cmd->argv[cmd->argc++] = word;
    }
    cmd->argv[cmd->argc] = NULL;

    return 0;
}


static pid_t execute_command(Command *cmd) {
    if (cmd->argc == 0) return -1;

    char *name = cmd->argv[0];

    if (strcmp(name, "exit") == 0) {
        builtin_exit(cmd);
        return -1;
    }
    if (strcmp(name, "chd") == 0) {
        builtin_chd(cmd);
        return -1;
    }
    if (strcmp(name, "route") == 0) {
        builtin_route(cmd);
        return -1;
    }

    char *exec_path = find_executable(name);
    if (!exec_path) {
        print_error();
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        print_error();
        free(exec_path);
        return -1;
    }

    if (pid == 0) {


        /* Redirección de salida*/
        if (cmd->outfile) {
            int fd = open(cmd->outfile,
                          O_WRONLY | O_CREAT | O_TRUNC,
                          0644);
            if (fd < 0) {
                print_error();
                exit(1);
            }
            /* Redirigir stdout y stderr al archivo */
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            close(fd);
        }

        execv(exec_path, cmd->argv);
        /* Si execv retorna, hubo error */
        print_error();
        exit(1);
    }

    /* Proceso padre: retorna el pid del hijo */
    free(exec_path);
    return pid;
}


static void process_line(char *line) {
    /* Eliminar '\n' al final */
    line = trim(line);
    if (strlen(line) == 0) return;

    /* Dividir por '&' para obtener comandos paralelos */
    /* Usamos strsep sobre una copia */
    char *line_copy = strdup(line);
    if (!line_copy) { print_error(); return; }

    /* Contamos los '&' y obtenemos cada sub-comando */
    #define MAX_CMDS 64
    char    *cmd_tokens[MAX_CMDS];
    int      num_cmds = 0;
    char    *rest     = line_copy;
    char    *tok;

    while ((tok = strsep(&rest, "&")) != NULL && num_cmds < MAX_CMDS) {
        cmd_tokens[num_cmds++] = tok;
    }

    /* Parsear y lanzar cada comando */
    pid_t pids[MAX_CMDS];
    int   pid_count = 0;

    for (int i = 0; i < num_cmds; i++) {
        char *ctok = trim(cmd_tokens[i]);
        if (strlen(ctok) == 0) continue;

        Command cmd;
        if (parse_single_command(ctok, &cmd) != 0) continue;
        if (cmd.argc == 0) continue;

        pid_t pid = execute_command(&cmd);
        if (pid > 0) {
            pids[pid_count++] = pid;
        }
    }

    /* Esperar a todos los hijos */
    for (int i = 0; i < pid_count; i++) {
        waitpid(pids[i], NULL, 0);
    }

    free(line_copy);
}


static void run_shell(FILE *input, int interactive) {
    char   *line = NULL;
    size_t  len  = 0;
    ssize_t nread;

    while (1) {
        if (interactive) {
            printf("wish> ");
            fflush(stdout);
        }

        nread = getline(&line, &len, input);

        if (nread == -1) {

            free(line);
            exit(0);
        }

        process_line(line);
    }
}

/* main */
int main(int argc, char *argv[]) {
    init_paths();

    if (argc == 1) {
        /* Modo interactivo */
        run_shell(stdin, 1);
    } else if (argc == 2) {
        /* Modo batch */
        FILE *batch = fopen(argv[1], "r");
        if (!batch) {
            print_error();
            exit(1);
        }
        run_shell(batch, 0);
        fclose(batch);
    } else {
        /* Más de un argumento: error y salir */
        print_error();
        exit(1);
    }

    return 0;
}
