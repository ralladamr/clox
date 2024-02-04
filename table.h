#ifndef clox_table
#define clox_table

#include <stdbool.h>
#include <stdint.h>

#include "object.h"
#include "value.h"

typedef struct
{
    String* key;
    Value value;
} Entry;

typedef struct
{
    int count;
    int capacity;
    Entry* entries;
} Table;

void init_table(Table* table);
void free_table(Table* table);
bool table_get(Table* table, String* key, Value* value);
bool table_set(Table* table, String* key, Value value);
bool table_delete(Table* table, String* key);
void table_add_all(Table* from, Table* to);
String* table_find_string(Table* table, const char* chars, int length, uint32_t hash);
void mark_table(Table* table);
void table_remove_white(Table* table);

#endif
