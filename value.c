#include <stdlib.h>

#include "memory.h"
#include "value.h"

void init_value_array(Value_array* array)
{
    array->capacity = 0;
    array->count = 0;
    array->values = NULL;
}

void free_value_arrray(Value_array* array)
{
    free_array(Value, array->values, array->capacity);
    init_value_array(array);
}

void write_value_array(Value_array* array, Value value)
{
    if (array->capacity < array->count + 1)
    {
        int old = array->capacity;
        array->capacity = grow_capacity(old);
        array->values = grow_array(Value, array->values, old, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

void print_value(Value value)
{
    printf("%g", value);
}
