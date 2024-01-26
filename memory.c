#include <stdint.h>
#include <stdlib.h>

#include "memory.h"
#include "value.h"

static void* reallocate(void*  pointer,
                        size_t old,
                        size_t new)
{
    if (new == 0)
    {
        free(pointer);
        return NULL;
    }

    void* result = realloc(pointer, new);
    if (result == NULL)
    {
        exit(1);
    }

    return result;
}

int* free_array_int(void* pointer, int count)
{
    size_t size = sizeof(int) * count;
    return (int*)reallocate(pointer, size, 0);
}

uint8_t* free_array_uint8_t(void* pointer, int count)
{
    size_t size = sizeof(uint8_t) * count;
    return (uint8_t*)reallocate(pointer, size, 0);
}

Value* free_array_value(void* pointer, int count)
{
    size_t size = sizeof(Value) * count;
    return (Value*)reallocate(pointer, size, 0);
}

int* grow_array_int(void* pointer,
                    int   old,
                    int   new)
{
    size_t old_size = sizeof(int) * old;
    size_t new_size = sizeof(int) * new;
    return (int*)reallocate(pointer, old_size, new_size);

}

uint8_t* grow_array_uint8_t(void* pointer,
                            int   old,
                            int   new)
{
    size_t old_size = sizeof(uint8_t) * old;
    size_t new_size = sizeof(uint8_t) * new;
    return (uint8_t*)reallocate(pointer, old_size, new_size);
}

Value* grow_array_value(void* pointer,
                        int   old,
                        int   new)
{
    size_t old_size = sizeof(Value) * old;
    size_t new_size = sizeof(Value) * new;
    return (Value*)reallocate(pointer, old_size, new_size);
}
