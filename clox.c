#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.h"
#include "debug.h"
#include "vm.h"

static void repl()
{
    char line[1024];
    bool cont = true;
    while (cont)
    {
        printf("> ");
        if (fgets(line, sizeof(line), stdin))
        {
            interpret(line);
        }
        else
        {
            printf("\n");
            cont = false;
        }
    }
}

static char* read_file(const char* path)
{
    // Get file size.
    FILE* file = fopen(path, "rb");
    if (file == NULL)
    {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }
    fseek(file, 0L, SEEK_END);
    size_t file_size = (size_t)ftell(file);
    rewind(file);

    // Read file to buffer.
    char* buffer = (char*)malloc(file_size + 1);
    if (buffer == NULL)
    {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }
    size_t bytes = fread((void*)buffer, sizeof(char), file_size, file);
    if (bytes < file_size)
    {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }
    buffer[bytes] = '\0';

    // Cleanup.
    fclose(file);
    return buffer;
}

static void run_file(const char* path)
{
    char* source = read_file(path);
    Interpret_result result = interpret(source);
    free(source);

    if (result == interpret_compile_error)
    {
        exit(65);
    }
    if (result == interpret_runtime_error)
    {
        exit(70);
    }
}

int main(int argc, char* argv[])
{
    init_VM();

    if (argc == 1)
    {
        repl();
    }
    else if (argc == 2)
    {
        run_file(argv[1]);
    }
    else
    {
        fprintf(stderr, "usage: clox [path]\n");
        exit(64);
    }

    free_VM();
    return 0;
}
