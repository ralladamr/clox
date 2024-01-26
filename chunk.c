#include <stdint.h>

#include "chunk.h"
#include "memory.h"
#include "value.h"

void init_chunk(Chunk* chunk)
{
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    init_value_array(&chunk->constants);
}

void free_chunk(Chunk* chunk)
{
    free_array(uint8_t, chunk->code, chunk->capacity);
    free_array(int, chunk->lines, chunk->capacity);
    free_value_arrray(&chunk->constants);
    init_chunk(chunk);
}

void write_chunk(Chunk*  chunk,
                 uint8_t byte,
                 int     line)
{
    if (chunk->capacity < chunk->count + 1) 
    {
        int old = chunk->capacity;
        int new = grow_capacity(old);
        chunk->capacity = new;
        chunk->code = grow_array(uint8_t, chunk->code, old, new);
        chunk->lines = grow_array(int, chunk->lines, old, new);
    }

    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

int add_constant(Chunk* chunk, Value value)
{
    write_value_array(&chunk->constants, value);
    return chunk->constants.count - 1;
}
