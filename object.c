#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

static Object* allocate_object(size_t size, Object_type type)
{
    Object* object = (Object*)allocate_void(size);
    object->type = type;
    object->next = vm.objects;
    vm.objects = object;
    return object;
}

static String* allocate_string(char* chars, int length, uint32_t hash)
{
    String* string = (String*)allocate_object(sizeof(String), obj_string);
    string->length = length;
    string->chars = chars;
    string->hash = hash;
    table_set(&vm.strings, string, nil_value());
    return string;
}

static uint32_t hash_string(const char* key, int length)
{
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++)
    {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

String* take_string(char* chars, int length)
{
    uint32_t hash = hash_string(chars, length);
    String* string;
    String* interned = table_find_string(&vm.strings, chars, length, hash);
    if (interned == NULL)
    {
        string = allocate_string(chars, length, hash);
    }
    else
    {
        free_array_char(chars, length);
        string = interned;
    }
    return string;
}

String* copy_string(const char* chars, int length)
{
    uint32_t hash = hash_string(chars, length);
    String* string;
    String* interned = table_find_string(&vm.strings, chars, length, hash);
    if (interned == NULL)
    {
        char* heap = allocate_char(length + 1);
        memcpy(heap, chars, length);
        heap[length] = '\0';
        string = allocate_string(heap, length, hash);
    }
    else
    {
        string = interned;
    }
    return string;
}

void print_object(Value value)
{
    switch (object_type(value))
    {
    case obj_string:
        printf("%s", as_cstring(value));
        break;
    default:
        break;
    }
}
