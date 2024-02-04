#ifndef clox_object
#define clox_object

#include <stdbool.h>
#include <stdint.h>

#include "chunk.h"
#include "table.h"
#include "value.h"

typedef enum
{
    obj_class,
    obj_bound_method,
    obj_function,
    obj_instance,
    obj_native,
    obj_closure,
    obj_upvalue,
    obj_string
} Object_type;

typedef struct Object
{
    Object_type type;
    bool is_marked;
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
    Object object;
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

typedef struct
{
    Object object;
    int arity;
    int upvalue_count;
    Chunk chunk;
    String* name;
} Function;

static inline bool is_function(Value value)
{
    return is_object_type(value, obj_function);
}

static inline Function* as_function(Value value)
{
    return (Function*)as_object(value);
}

typedef struct
{
    Object object;
    Value(*function)(int, Value*);
} Native;

static inline bool is_native(Value value)
{
    return is_object_type(value, obj_native);
}

static inline Value(*as_native(Value value))(int, Value*)
{
    return ((Native*)as_object(value))->function;
}

typedef struct Upvalue
{
    Object object;
    Value* location;
    Value closed;
    struct Upvalue* next;
} Upvalue;

typedef struct
{
    Object object;
    Function* function;
    Upvalue** upvalues;
    int upvalue_count;
} Closure;

static inline bool is_closure(Value value)
{
    return is_object_type(value, obj_closure);
}

static inline Closure* as_closure(Value value)
{
    return (Closure*)as_object(value);
}

typedef struct
{
    Object object;
    String* name;
    Table methods;
} Class;

static inline bool is_class(Value value)
{
    return is_object_type(value, obj_class);
}

static inline Class* as_class(Value value)
{
    return (Class*)as_object(value);
}

typedef struct
{
    Object object;
    Class* class;
    Table fields;
} Instance;

static inline bool is_instance(Value value)
{
    return is_object_type(value, obj_instance);
}

static inline Instance* as_instance(Value value)
{
    return (Instance*)as_object(value);
}

typedef struct
{
    Object object;
    Value receiver;
    Closure* method;
} Bound_method;

static inline bool is_bound_method(Value value)
{
    return is_object_type(value, obj_bound_method);
}

static inline Bound_method* as_bound_method(Value value)
{
    return (Bound_method*)as_object(value);
}

Class* new_class(String* name);
Bound_method* new_bound_method(Value receiver, Closure* method);
Upvalue* new_upvalue(Value* slot);
Closure* new_closure(Function* function);
Function* new_function();
Instance* new_instance(Class* class);
Native* new_native(Value(*function)(int, Value*));
String* take_string(char* chars, int length);
String* copy_string(const char* chars, int length);
void print_object(Value value);

#endif
