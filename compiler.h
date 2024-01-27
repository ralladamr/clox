#ifndef clox_compiler
#define clox_compiler

#include <stdbool.h>

#include "chunk.h"

bool compile(const char* source, Chunk* chunk);

#endif
