#ifndef clox_memory
#define clox_memory

#include <stdint.h>

#include "value.h"

static inline int grow_capacity(int capacity)
{
    return capacity < 8 ? 8 : (capacity * 2);
}

int* free_array_int(void* pointer, int count);
uint8_t* free_array_uint8_t(void* pointer, int count);
Value* free_array_value(void* pointer, int count);

int* grow_array_int(void* pointer,
                    int   old,
                    int   new);
uint8_t* grow_array_uint8_t(void* pointer,
                            int   old,
                            int   new);
Value* grow_array_value(void* pointer,
                        int   old,
                        int   new);

#endif clox_memory
