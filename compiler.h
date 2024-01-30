#ifndef clox_compiler
#define clox_compiler

#include <stdbool.h>
#include <stdint.h>

#include "chunk.h"

#define UINT8_COUNT (UINT8_MAX + 1)

bool compile(const char* source, Chunk* chunk);

#endif
