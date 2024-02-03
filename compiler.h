#ifndef clox_compiler
#define clox_compiler

#include <stdbool.h>
#include <stdint.h>

#include "chunk.h"
#include "object.h"

enum Compiler_param
{
    variables_max = UINT8_MAX + 1
};

Function* compile(const char* source);

#endif
