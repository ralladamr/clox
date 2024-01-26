#ifndef clox_value
#define clox_value

typedef double Value;

typedef struct
{
    int capacity;
    int count;
    Value* values;
} Value_array;

void init_value_array(Value_array* array);
void free_value_arrray(Value_array* array);
void write_value_array(Value_array* array, Value value);
void print_value(Value value);

#endif
