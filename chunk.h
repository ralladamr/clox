#ifndef clox_chunk
#define clox_chunk

#include <stdint.h>

typedef enum 
{
    op_return,
} Op_code;

typedef struct 
{
    int count;
    int capacity;
    uint8_t* code;
} Chunk;

void init_chunk(Chunk* chunk);
void free_chunk(Chunk* chunk);
void write_chunk(Chunk* chunk, uint8_t byte);

#endif
