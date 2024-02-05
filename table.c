#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

static const double table_max_load = 0.75;

static Entry* find_entry(Entry* entries, int capacity, String* key)
{
    uint32_t index = key->hash & (capacity - 1);
    Entry* tombstone = NULL;
    Entry* entry = NULL;
    bool found = false;
    while (!found)
    {
        entry = &entries[index];
        if (entry->key == NULL)
        {
            if (is_nil(entry->value))
            {
                found = true;
                if (tombstone != NULL)
                {
                    entry = tombstone;
                }
            }
            else if (tombstone == NULL)
            {
                tombstone = entry;
            }
        }
        else if (entry->key == key)
        {
            found = true;
        }
        index = (index + 1) & (capacity - 1);
    }
    return entry;
}

static void adjust_capacity(Table* table, int capacity)
{
    Entry* entries = allocate_entry(capacity);
    for (int i = 0; i < capacity; i++)
    {
        entries[i].key = NULL;
        entries[i].value = nil_value();
    }
    table->count = 0;
    for (int i = 0; i < table->capacity; i++)
    {
        Entry* entry = &table->entries[i];
        if (entry->key != NULL)
        {
            Entry* dest = find_entry(entries, capacity, entry->key);
            dest->key = entry->key;
            dest->value = entry->value;
            table->count++;
        }
    }
    free_array_entry(table->entries, table->capacity);
    table->entries = entries;
    table->capacity = capacity;
}

void init_table(Table* table)
{
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void free_table(Table* table)
{
    free_array_entry(table->entries, table->capacity);
    init_table(table);
}

bool table_get(Table* table, String* key, Value* value)
{
    bool found = false;
    if (table->count != 0)
    {
        Entry* entry = find_entry(table->entries, table->capacity, key);
        if (entry->key != NULL)
        {
            *value = entry->value;
            found = true;
        }
    }
    return found;
}

bool table_set(Table* table, String* key, Value value)
{
    if (table->count + 1 > table->capacity * table_max_load)
    {
        int capacity = grow_capacity(table->capacity);
        adjust_capacity(table, capacity);
    }
    Entry* entry = find_entry(table->entries, table->capacity, key);
    bool is_new = entry->key == NULL;
    if (is_new && is_nil(entry->value))
    {
        table->count++;
    }
    entry->key = key;
    entry->value = value;
    return is_new;
}

bool table_delete(Table* table, String* key)
{
    bool deleted = false;
    if (table->count != 0)
    {
        Entry* entry = find_entry(table->entries, table->capacity, key);
        if (entry->key != NULL)
        {
            // Place a tombstone in the entry.
            entry->key = NULL;
            entry->value = bool_value(true);
            deleted = true;
        }
    }
    return deleted;
}

void table_add_all(Table* from, Table* to)
{
    for (int i = 0; i < from->capacity; i++)
    {
        Entry* entry = &from->entries[i];
        if (entry->key != NULL)
        {
            table_set(to, entry->key, entry->value);
        }
    }
}

String* table_find_string(Table* table, const char* chars, int length, uint32_t hash)
{
    String* string = NULL;
    if (table->count != 0)
    {
        uint32_t index = hash & (table->capacity - 1);
        bool stop = false;
        while (!stop)
        {
            Entry* entry = &table->entries[index];
            if (entry->key == NULL)
            {
                if (is_nil(entry->value))
                {
                    stop = true;
                }
            }
            else if (entry->key->length == length && entry->key->hash == hash
                && memcmp(entry->key->chars, chars, length) == 0)
            {
                string = entry->key;
                stop = true;
            }
            index = (index + 1) & (table->capacity - 1);
        }
    }
    return string;
}

void mark_table(Table* table)
{
    for (int i = 0; i < table->capacity; i++)
    {
        Entry* entry = &table->entries[i];
        mark_object((Object*)entry->key);
        mark_value(entry->value);
    }
}

void table_remove_white(Table* table)
{
    for (int i = 0; i < table->capacity; i++)
    {
        Entry* entry = &table->entries[i];
        if (entry->key != NULL && !entry->key->object.is_marked)
        {
            table_delete(table, entry->key);
        }
    }
}
