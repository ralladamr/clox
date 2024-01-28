#ifndef clox_object
#define clox_object

#include <stdbool.h>
#include <stdint.h>

#include "value.h"

typedef enum
{
    obj_string
} Object_type;

typedef struct Object
{
    Object_type type;
    Object* next;
} Object;

static inline bool is_object_type(Value value, Object_type type)
{
    return is_object(value) && as_object(value)->type == type;
}

static inline Object_type object_type(Value value)
{
    return as_object(value)->type;
}

typedef struct String
{
    Object obj;
    int length;
    char* chars;
    uint32_t hash;
} String;

static inline bool is_string(Value value)
{
    return is_object_type(value, obj_string);
}

static inline String* as_string(Value value)
{
    return (String*)as_object(value);
}

static inline char* as_cstring(Value value)
{
    return as_string(value)->chars;
}

String* take_string(char* chars, int length);
String* copy_string(const char* chars, int length);
void print_object(Value value);

#endif
