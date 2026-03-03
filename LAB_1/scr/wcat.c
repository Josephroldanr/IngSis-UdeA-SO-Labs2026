#include <stdio.h>
#include <stdlib.h>

FILE *openFile(char *ruta) {
    FILE *file = fopen(ruta, "r");

    if (file == NULL) {
        printf("wcat: cannot open file\n");
        exit(1);
    }

    return file;
}

void printFile(FILE *file) {
    char buffer[1024];

    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        printf("%s", buffer);
    }
}

int main(int argc, char *argv[]) {
    // Si no hay archivos, salir con éxito
    if (argc < 2) {
        return 0;
    }

    // Imprimir cada archivo en orden
    for (int i = 1; i < argc; i++) {
        FILE *file = openFile(argv[i]);
        printFile(file);
        fclose(file);
    }

    return 0;
}