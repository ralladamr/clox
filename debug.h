#ifndef clox_debug
#define clox_debug

#define DEBUG_TRACE_EXECUTION

#include "chunk.h"

void disassemble_chunk(Chunk* chunk, const char* name);
int disassemble_instruction(Chunk* chunk, int offset);

#endif
