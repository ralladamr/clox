#include <malloc.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "compiler.h"
#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif

enum GC_parameter
{
    gc_heap_grow_factor = 2
};

static void* reallocate(void* pointer, size_t old, size_t new)
{
    (void)old;
    vm.bytes_allocated += new - old;
    void* result;
    if (new > old)
    {
#ifdef DEBUG_STRESS_GC
        collect_garbage();
#endif
        if (vm.bytes_allocated > vm.next_GC)
        {
            collect_garbage();
        }
    }
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

static void free_object(Object* object)
{
#ifdef DEBUG_LOG_GC
    printf("%p free type %d\n", (void*)object, object->type);
#endif
    switch (object->type)
    {
    case obj_upvalue:
    {
        Upvalue* upvalue = (Upvalue*)object;
        reallocate(object, sizeof(Upvalue), 0);
        break;
    }
    case obj_closure:
    {
        Closure* closure = (Closure*)object;
        free_array_upvalues(closure->upvalues, closure->upvalue_count);
        reallocate(object, sizeof(Closure), 0);
        break;
    }
    case obj_function:
    {
        Function* function = (Function*)object;
        free_chunk(&function->chunk);
        reallocate(object, sizeof(Function), 0);
        break;
    }
    case obj_native:
    {
        Native* native = (Native*)object;
        reallocate(object, sizeof(Native), 0);
        break;
    }
    case obj_string:
    {
        String* string = (String*)object;
        free_array_char(string->chars, string->length + 1);
        reallocate(object, sizeof(String), 0);
        break;
    }
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

Upvalue** allocate_upvalues(int count)
{
    return (Upvalue**)reallocate(NULL, 0, sizeof(Upvalue*) * count);
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
void free_array_upvalues(Upvalue** pointer, int count)
{
    size_t size = sizeof(Upvalue*) * count;
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
    free(vm.gray_stack);
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

void mark_object(Object* object)
{
    if (object != NULL && !object->is_marked)
    {
#ifdef DEBUG_LOG_GC
        printf("%p mark ", (void*)object);
        print_value(object_value(object));
        printf("\n");
#endif
        object->is_marked = true;
        if (vm.gray_capacity < vm.gray_count + 1)
        {
            vm.gray_capacity = grow_capacity(vm.gray_capacity);
            Object** grays = (Object**)realloc(vm.gray_stack, sizeof(Object*) * vm.gray_capacity);
            if (grays == NULL)
            {
                exit(1);
            }
            vm.gray_stack = grays;
        }
        vm.gray_stack[vm.gray_count++] = object;
    }
}

void mark_value(Value value)
{
    if (is_object(value))
    {
        mark_object(as_object(value));
    }
}

static void mark_array(Value_array* array)
{
    for (int i = 0; i < array->count; i++)
    {
        mark_value(array->values[i]);
    }
}

static void mark_roots()
{
    for (Value* slot = vm.stack; slot < vm.stack_top; slot++)
    {
        mark_value(*slot);
    }
    for (int i = 0; i < vm.frame_count; i++)
    {
        mark_object((Object*)vm.frames[i].closure);
    }
    for (Upvalue* uv = vm.open_upvalues; uv != NULL; uv = uv->next)
    {
        mark_object((Object*)uv);
    }
    mark_table(&vm.globals);
    mark_compiler_roots();
}

static void blacken_object(Object* object)
{
#ifdef DEBUG_LOG_GC
    printf("%p blacken ", (void*)object);
    print_value(object_value(object));
    printf("\n");
#endif
    switch (object->type)
    {
    case obj_upvalue:
        mark_value(((Upvalue*)object)->closed);
        break;
    case obj_function:
    {
        Function* function = (Function*)object;
        mark_object((Object*)function->name);
        mark_array(&function->chunk.constants);
        break;
    }
    case obj_closure:
    {
        Closure* closure = (Closure*)object;
        mark_object((Object*)closure->function);
        for (int i = 0; i < closure->upvalue_count; i++)
        {
            mark_object((Object*)closure->upvalues[i]);
        }
    }
    case obj_native:
    case obj_string:
    default:
        break;
    }
}

static void trace_references()
{
    while (vm.gray_count > 0)
    {
        Object* object = vm.gray_stack[--vm.gray_count];
        blacken_object(object);
    }
}

static void sweep()
{
    Object* previous = NULL;
    Object* object = vm.objects;
    while (object != NULL)
    {
        if (object->is_marked)
        {
            object->is_marked = false;
            previous = object;
            object = object->next;
        }
        else
        {
            Object* unreached = object;
            object = object->next;
            if (previous != NULL)
            {
                previous->next = object;
            }
            else
            {
                vm.objects = object;
            }
            free_object(unreached);
        }
    }
}

void collect_garbage()
{
#ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
    size_t before = vm.bytes_allocated;
#endif
    mark_roots();
    trace_references();
    table_remove_white(&vm.strings);
    sweep();
    vm.next_GC = vm.bytes_allocated * gc_heap_grow_factor;
#ifdef DEBUG_LOG_GC
    printf("-- gc end\n");
    printf("   collected %zu bytes (from %zu to %zu) next at %zu\n", before - vm.bytes_allocated,
        before, vm.bytes_allocated, vm.next_GC);
#endif
}
