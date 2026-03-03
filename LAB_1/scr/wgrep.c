#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void processFile(FILE *file, char *pattern) {
    char *line = NULL;
    size_t len = 0;

    while (getline(&line, &len, file) != -1) {
        if (strstr(line, pattern) != NULL) {
            printf("%s", line);
        }
    }

    free(line);
}

int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("wgrep: searchterm [file ...]\n");
        exit(1);
    }

    char *pattern = argv[1];

    // Solo patrón → leer stdin
    if (argc == 2) {
        processFile(stdin, pattern);
        return 0;
    }

    // Hay archivos
    for (int i = 2; i < argc; i++) {
        FILE *file = fopen(argv[i], "r");
        if (file == NULL) {
            printf("wgrep: cannot open file\n");
            exit(1);
        }

        processFile(file, pattern);
        fclose(file);
    }

    return 0;
}     