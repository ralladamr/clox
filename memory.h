#ifndef clox_memory
#define clox_memory

#include <stddef.h>
#include <stdint.h>

#include "table.h"
#include "value.h"

static inline int grow_capacity(int capacity)
{
    return capacity < 8 ? 8 : (capacity * 2);
}

char* allocate_char(int count);
Entry* allocate_entry(int count);
void* allocate_void(size_t size);

void free_array_char(char* pointer, int count);
void free_array_entry(Entry* pointer, int count);
void free_array_int(int* pointer, int count);
void free_array_uint8_t(uint8_t* pointer, int count);
void free_array_value(Value* pointer, int count);
void free_objects();

int* grow_array_int(int* pointer, int old, int new);
uint8_t* grow_array_uint8_t(uint8_t* pointer, int old, int new);
Value* grow_array_value(Value* pointer, int old, int new);

#endif
