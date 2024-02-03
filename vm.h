#ifndef clox_vm
#define clox_vm

#include <stdint.h>

#include "chunk.h"
#include "object.h"
#include "table.h"
#include "value.h"

enum VM_parameter
{
    frames_max = 64,
    stack_max = frames_max * (UINT8_MAX + 1)
};

typedef struct
{
    Function* function;
    uint8_t* ip;
    Value* slots;
} Call_frame;

typedef struct
{
    Call_frame frames[frames_max];
    int frame_count;
    Value stack[stack_max];
    Value* stack_top;
    Table globals;
    Table strings;
    Object* objects;
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
