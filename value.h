#ifndef clox_value
#define clox_value

#include <stdbool.h>

typedef struct Obj Obj;

typedef enum
{
    val_bool,
    val_nil,
    val_number,
    val_obj
} Value_type;

typedef struct
{
    Value_type type;
    union
    {
        bool boolean;
        double number;
        Obj* obj;
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

static inline bool is_obj(Value value)
{
    return value.type == val_obj;
}

static inline bool as_bool(Value value)
{
    return value.as.boolean;
}

static inline double as_number(Value value)
{
    return value.as.number;
}

static inline Obj* as_obj(Value value)
{
    return value.as.obj;
}

static inline Value bool_value(bool value)
{
    Value v = { val_bool, {.boolean = value } };
    return v;
}

static inline Value nil_value()
{
    Value v = { val_nil, {.number = 0 } };
    return v;
}

static inline Value number_value(double value)
{
    Value v = { val_number, {.number = value } };
    return v;
}

static inline Value obj_value(Obj* object)
{
    Value v = { val_obj, {.obj = (Obj*)object } };
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
