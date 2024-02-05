#ifndef clox_chunk
#define clox_chunk

#include <stdint.h>

#include "value.h"

typedef enum
{
    op_constant,
    op_nil,
    op_true,
    op_false,
    op_negate,
    op_pop,
    op_get_local,
    op_set_local,
    op_get_upvalue,
    op_set_upvalue,
    op_close_upvalue,
    op_get_global,
    op_define_global,
    op_set_global,
    op_get_property,
    op_set_property,
    op_method,
    op_equal,
    op_greater,
    op_less,
    op_add,
    op_subtract,
    op_multiply,
    op_divide,
    op_not,
    op_print,
    op_jump_if_false,
    op_jump,
    op_loop,
    op_call,
    op_invoke,
    op_closure,
    op_return,
    op_class
} Op_code;

typedef struct
{
    int count;
    int capacity;
    uint8_t* code;
    int* lines;
    Value_array constants;
} Chunk;

void init_chunk(Chunk* chunk);
void free_chunk(Chunk* chunk);
void write_chunk(Chunk* chunk, uint8_t byte, int line);
int add_constant(Chunk* chunk, Value value);

#endif
