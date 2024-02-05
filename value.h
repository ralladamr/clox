#ifndef clox_value
#define clox_value

#include <stdbool.h>
#include <string.h>

typedef struct Object Object;

#ifdef NAN_BOXING

typedef uint64_t Value;

enum Value_tag
{
    tag_nil = 1,
    tag_false = 2,
    tag_true = 3
};

static inline uint64_t qnan()
{
    return 0x7ffc000000000000ull;
}

static inline uint64_t sign_bit()
{
    return 0x8000000000000000ull;
}

static inline Value false_value()
{
    return (Value)(uint64_t)(qnan() | tag_false);
}

static inline Value true_value()
{
    return (Value)(uint64_t)(qnan() | tag_true);
}

static inline bool as_bool(Value value)
{
    return value == true_value();
}

static inline double as_number(Value value)
{
    double num;
    memcpy(&num, &value, sizeof(Value));
    return num;
}

static inline Object* as_object(Value value)
{
    return (Object*)(uintptr_t)(value & ~(sign_bit() | qnan()));
}

static inline Value bool_value(bool b)
{
    return b ? true_value() : false_value();
}

static inline Value nil_value()
{
    return (Value)(uint64_t)(qnan() | tag_nil);
}

static inline Value number_value(double num)
{
    Value value;
    memcpy(&value, &num, sizeof(double));
    return value;
}

static inline Value object_value(Object* object)
{
    return (Value)(sign_bit() | qnan() | (uint64_t)(uintptr_t)object);
}

static inline bool is_bool(Value value)
{
    return (value | 1) == true_value();
}

static inline bool is_nil(Value value)
{
    return value == nil_value();
}

static inline bool is_number(Value value)
{
    return (value & qnan()) != qnan();
}

static inline bool is_object(Value value)
{
    return (value & (qnan() | sign_bit())) == (qnan() | sign_bit());
}

#else

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

#endif

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
