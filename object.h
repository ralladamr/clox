#ifndef clox_object
#define clox_object

#include <stdbool.h>

#include "value.h"

typedef enum
{
    obj_string
} Obj_type;

typedef struct Obj
{
    Obj_type type;
    Obj* next;
} Obj;

static inline bool is_obj_type(Value value, Obj_type type)
{
    return is_obj(value) && as_obj(value)->type == type;
}

static inline Obj_type obj_type(Value value)
{
    return as_obj(value)->type;
}

typedef struct Obj_string
{
    Obj obj;
    int length;
    char* chars;
} Obj_string;

static inline bool is_string(Value value)
{
    return is_obj_type(value, obj_string);
}

static inline Obj_string* as_string(Value value)
{
    return (Obj_string*)as_obj(value);
}

static inline char* as_cstring(Value value)
{
    return as_string(value)->chars;
}

Obj_string* take_string(char* chars, int length);
Obj_string* copy_string(const char* chars, int length);
void print_obj(Value value);

#endif
