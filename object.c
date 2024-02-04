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
    object->is_marked = false;
    object->next = vm.objects;
    vm.objects = object;
#ifdef DEBUG_LOG_GC
    printf("%p allocate %zu for %d\n", (void*)object, size, type);
#endif
    return object;
}

static String* allocate_string(char* chars, int length, uint32_t hash)
{
    String* string = (String*)allocate_object(sizeof(String), obj_string);
    string->length = length;
    string->chars = chars;
    string->hash = hash;
    push(object_value((Object*)string));
    table_set(&vm.strings, string, nil_value());
    pop();
    return string;
}

Class* new_class(String* name)
{
    Class* class = (Class*)allocate_object(sizeof(Class), obj_class);
    class->name = name;
    return class;
}

Upvalue* new_upvalue(Value* slot)
{
    Upvalue* upvalue = (Upvalue*)allocate_object(sizeof(Upvalue), obj_upvalue);
    upvalue->location = slot;
    upvalue->closed = nil_value();
    upvalue->next = NULL;
    return upvalue;
}

Closure* new_closure(Function* function)
{
    Closure* closure = (Closure*)allocate_object(sizeof(Closure), obj_closure);
    closure->function = function;
    Upvalue** upvalues = allocate_upvalues(function->upvalue_count);
    for (int i = 0; i < function->upvalue_count; i++)
    {
        upvalues[i] = NULL;
    }
    closure->upvalues = upvalues;
    closure->upvalue_count = function->upvalue_count;
    return closure;
}

Function* new_function()
{
    Function* function = (Function*)allocate_object(sizeof(Function), obj_function);
    function->arity = 0;
    function->upvalue_count = 0;
    function->name = NULL;
    init_chunk(&function->chunk);
    return function;
}

Instance* new_instance(Class* class)
{
    Instance* instance = (Instance*)allocate_object(sizeof(Instance), obj_instance);
    instance->class = class;
    init_table(&instance->fields);
    return instance;
}

Native* new_native(Value(*function)(int, Value*))
{
    Native* native = (Native*)allocate_object(sizeof(Native), obj_native);
    native->function = function;
    return native;
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

static void print_function(Function* function)
{
    if (function->name == NULL)
    {
        printf("<script>");
    }
    else
    {
        printf("<fn %s>", function->name->chars);
    }
}

void print_object(Value value)
{
    switch (object_type(value))
    {
    case obj_class:
        printf("%s", as_class(value)->name->chars);
        break;
    case obj_upvalue:
        printf("upvalue");
        break;
    case obj_closure:
        print_function(as_closure(value)->function);
        break;
    case obj_function:
        print_function(as_function(value));
        break;
    case obj_instance:
        printf("%s instance", as_instance(value)->class->name->chars);
        break;
    case obj_native:
        printf("<native fn>");
        break;
    case obj_string:
        printf("%s", as_cstring(value));
        break;
    default:
        break;
    }
}
