#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

static Obj* allocate_obj(size_t size, Obj_type type)
{
    Obj* obj = (Obj*)allocate_void(size);
    obj->type = type;
    obj->next = vm.objects;
    vm.objects = obj;
    return obj;
}

static Obj_string* allocate_string(char* chars, int length, uint32_t hash)
{
    Obj_string* string = (Obj_string*)allocate_obj(sizeof(Obj_string*), obj_string);
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

Obj_string* take_string(char* chars, int length)
{
    uint32_t hash = hash_string(chars, length);
    Obj_string* string;
    Obj_string* interned = table_find_string(&vm.strings, chars, length, hash);
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

Obj_string* copy_string(const char* chars, int length)
{
    uint32_t hash = hash_string(chars, length);
    Obj_string* string;
    Obj_string* interned = table_find_string(&vm.strings, chars, length, hash);
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

void print_obj(Value value)
{
    switch (obj_type(value))
    {
    case obj_string:
        printf("%s", as_cstring(value));
        break;
    default:
        break;
    }
}
