#ifndef clox_vm
#define clox_vm

#include <stdint.h>

#include "chunk.h"

typedef struct
{
    Chunk*   chunk;
    uint8_t* ip;
} VM;

typedef enum
{
    interpret_continue,
    interpret_ok,
    interpret_compile_error,
    interpret_runtime_error
} Interpret_result;

void init_VM();
void free_VM();
Interpret_result interpret(Chunk* chunk);

#endif
