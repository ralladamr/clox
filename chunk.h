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
    op_get_global,
    op_define_global,
    op_set_global,
    op_equal,
    op_greater,
    op_less,
    op_add,
    op_subtract,
    op_multiply,
    op_divide,
    op_not,
    op_print,
    op_return,
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
