#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
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

static Obj_string* allocate_obj_string()
{
    return (Obj_string*)allocate_obj(sizeof(Obj_string*), obj_string);
}

static Obj_string* allocate_string(char* chars, int length)
{
    Obj_string* string = allocate_obj_string();
    string->length = length;
    string->chars = chars;
    return string;
}

Obj_string* take_string(char* chars, int length)
{
    return allocate_string(chars, length);
}

Obj_string* copy_string(const char* chars, int length)
{
    char* heap = allocate_char(length + 1);
    memcpy(heap, chars, length);
    heap[length] = '\0';
    return allocate_string(heap, length);
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
