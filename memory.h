#ifndef clox_memory
#define clox_memory

#include <stddef.h>

#define free_array(type, pointer, old) ((type*)reallocate(pointer, sizeof(type) * (old), 0))
#define grow_array(type, pointer, old, new) ((type*)reallocate(pointer, sizeof(type) * (old), sizeof(type) * new))

static inline int grow_capacity(int capacity)
{
    return capacity < 8 ? 8 : (capacity * 2);
}

void* reallocate(void*  pointer, 
                 size_t old,
                 size_t new);

#endif clox_memory
