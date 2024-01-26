#ifndef clox_chunk
#define clox_chunk

#include <stdint.h>

#include "value.h"

typedef enum 
{
    op_constant,
    op_return,
} Op_code;

typedef struct 
{
    int         count;
    int         capacity;
    uint8_t*    code;
    int*        lines;
    Value_array constants;
} Chunk;

void init_chunk(Chunk* chunk);
void free_chunk(Chunk* chunk);
void write_chunk(Chunk*  chunk, 
                 uint8_t byte,
                 int     line);
int add_constant(Chunk* chunk, Value value);

#endif
