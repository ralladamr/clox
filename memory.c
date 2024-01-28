#include <malloc.h>
#include <stdint.h>
#include <stdlib.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

static void* reallocate(void* pointer, size_t old, size_t new)
{
    void* result;
    if (new == 0)
    {
        free(pointer);
        result = NULL;
    }
    else
    {
        result = realloc(pointer, new);
        if (result == NULL)
        {
            exit(1);
        }
    }
    return result;
}

static void free_string(String* pointer)
{
    reallocate(pointer, sizeof(String), 0);
}

static void free_object(Object* object)
{
    switch (object->type)
    {
    case obj_string:
        String* string = (String*)object;
        free_array_char(string->chars, string->length + 1);
        free_string(string);
    default:
        break;
    }
}

char* allocate_char(int count)
{
    return (char*)reallocate(NULL, 0, sizeof(char) * count);
}

Entry* allocate_entry(int count)
{
    return (Entry*)reallocate(NULL, 0, sizeof(Entry) * count);
}

void* allocate_void(size_t size)
{
    return reallocate(NULL, 0, size);
}

void free_array_char(char* pointer, int count)
{
    size_t size = sizeof(char) * count;
    reallocate(pointer, size, 0);
}

void free_array_entry(Entry* pointer, int count)
{
    size_t size = sizeof(Entry) * count;
    reallocate(pointer, size, 0);
}

void free_array_int(int* pointer, int count)
{
    size_t size = sizeof(int) * count;
    reallocate(pointer, size, 0);
}

void free_array_uint8_t(uint8_t* pointer, int count)
{
    size_t size = sizeof(uint8_t) * count;
    reallocate(pointer, size, 0);
}

void free_array_value(Value* pointer, int count)
{
    size_t size = sizeof(Value) * count;
    reallocate(pointer, size, 0);
}

void free_objects()
{
    Object* object = vm.objects;
    while (object != NULL)
    {
        Object* next = object->next;
        free_object(object);
        object = next;
    }
}

int* grow_array_int(int* pointer, int old, int new)
{
    size_t old_size = sizeof(int) * old;
    size_t new_size = sizeof(int) * new;
    return (int*)reallocate(pointer, old_size, new_size);

}

uint8_t* grow_array_uint8_t(uint8_t* pointer, int old, int new)
{
    size_t old_size = sizeof(uint8_t) * old;
    size_t new_size = sizeof(uint8_t) * new;
    return (uint8_t*)reallocate(pointer, old_size, new_size);
}

Value* grow_array_value(Value* pointer, int old, int new)
{
    size_t old_size = sizeof(Value) * old;
    size_t new_size = sizeof(Value) * new;
    return (Value*)reallocate(pointer, old_size, new_size);
}
