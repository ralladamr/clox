#ifndef clox_compiler
#define clox_compiler

#include <stdbool.h>
#include <stdint.h>

#include "chunk.h"
#include "object.h"

#define UINT8_COUNT (UINT8_MAX + 1)

Function* compile(const char* source);

#endif
