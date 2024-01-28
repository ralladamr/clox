#ifndef clox_value
#define clox_value

#include <stdbool.h>

typedef struct Object Object;

typedef enum
{
    val_bool,
    val_nil,
    val_number,
    val_object
} Value_type;

typedef struct
{
    Value_type type;
    union
    {
        bool boolean;
        double number;
        Object* object;
    } as;
} Value;

static inline bool is_bool(Value value)
{
    return value.type == val_bool;
}

static inline bool is_nil(Value value)
{
    return value.type == val_nil;
}

static inline bool is_number(Value value)
{
    return value.type == val_number;
}

static inline bool is_object(Value value)
{
    return value.type == val_object;
}

static inline bool as_bool(Value value)
{
    return value.as.boolean;
}

static inline double as_number(Value value)
{
    return value.as.number;
}

static inline Object* as_object(Value value)
{
    return value.as.object;
}

static inline Value bool_value(bool value)
{
    Value v = {val_bool, {.boolean = value}};
    return v;
}

static inline Value nil_value()
{
    Value v = {val_nil, {.number = 0}};
    return v;
}

static inline Value number_value(double value)
{
    Value v = {val_number, {.number = value}};
    return v;
}

static inline Value object_value(Object* object)
{
    Value v = {val_object, {.object = object}};
    return v;
}

typedef struct
{
    int capacity;
    int count;
    Value* values;
} Value_array;

bool values_equal(Value a, Value b);
void init_value_array(Value_array* array);
void free_value_arrray(Value_array* array);
void write_value_array(Value_array* array, Value value);
void print_value(Value value);

#endif
