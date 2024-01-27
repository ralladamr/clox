#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "memory.h"
#include "value.h"

static void grow(Value_array* array)
{
    int old = array->capacity;
    array->capacity = grow_capacity(old);
    array->values = grow_array_value(array->values, old, array->capacity);
}

bool values_equal(Value a, Value b)
{
    bool result = false;
    if (a.type == b.type)
    {
        switch (a.type)
        {
        case val_bool:
            result = as_bool(a) == as_bool(b);
            break;
        case val_nil:
            result = true;
            break;
        case val_number:
            result = as_number(a) == as_number(b);
            break;
        default:
            break;
        }
    }
    return result;
}

void init_value_array(Value_array* array)
{
    array->capacity = 0;
    array->count = 0;
    array->values = NULL;
}

void free_value_arrray(Value_array* array)
{
    free_array_value(array->values, array->capacity);
    init_value_array(array);
}

void write_value_array(Value_array* array, Value value)
{
    if (array->capacity < array->count + 1)
    {
        grow(array);
    }

    array->values[array->count] = value;
    array->count++;
}

void print_value(Value value)
{
    switch (value.type)
    {
    case val_bool:
        printf(as_bool(value) ? "true" : "false");
        break;
    case val_nil:
        printf("nil");
        break;
    case val_number:
        printf("%g", as_number(value));
        break;
    }
}
