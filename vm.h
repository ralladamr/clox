#ifndef clox_vm
#define clox_vm

#include <stdint.h>

#include "chunk.h"
#include "value.h"

enum Stack_parameter
{
    stack_max = 256
};

typedef struct
{
    Chunk* chunk;
    uint8_t* ip;
    Value stack[stack_max];
    Value* stack_top;
    Obj* objects;
} VM;

extern VM vm;

typedef enum
{
    interpret_continue,
    interpret_ok,
    interpret_compile_error,
    interpret_runtime_error
} Interpret_result;

void init_VM();
void free_VM();
Interpret_result interpret(const char* source);
void push(Value value);
Value pop();

#endif
