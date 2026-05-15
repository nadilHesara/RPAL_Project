#include "utils.h"

#include <stdio.h>
#include <stdlib.h>

char *read_file_contents(const char *file_name)
{
    FILE *file = NULL;
    char *buffer = NULL;
    long size = 0;
    size_t bytes_read = 0;

    file = fopen(file_name, "rb");
    if (file == NULL) {
        perror(file_name);
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        perror(file_name);
        fclose(file);
        return NULL;
    }

    size = ftell(file);
    if (size < 0) {
        perror(file_name);
        fclose(file);
        return NULL;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        perror(file_name);
        fclose(file);
        return NULL;
    }

    buffer = (char *)malloc((size_t)size + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Error: out of memory while reading '%s'\n", file_name);
        fclose(file);
        return NULL;
    }

    bytes_read = fread(buffer, 1, (size_t)size, file);
    if (bytes_read != (size_t)size) {
        fprintf(stderr, "Error: failed to read '%s'\n", file_name);
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[bytes_read] = '\0';
    fclose(file);
    return buffer;
}
