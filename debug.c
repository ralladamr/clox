#include <stdint.h>
#include <stdio.h>

#include "chunk.h"
#include "debug.h"
#include "value.h"

static int constant_instruction(const char* name, Chunk* chunk, int offset)
{
    uint8_t constant = chunk->code[offset + 1];
    printf("%-16s %4d '", name, constant);
    print_value(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 2;
}

static int simple_instruction(const char* name, int offset)
{
    printf("%s\n", name);
    return offset + 1;
}

static int unknown_instruction(uint8_t instruction, int offset)
{
    printf("Unknown opcode %d\n", instruction);
    return offset + 1;
}

void disassemble_chunk(Chunk* chunk, const char* name)
{
    printf("== %s ==\n", name);
    int offset = 0;
    while (offset < chunk->count)
    {
        offset = disassemble_instruction(chunk, offset);
    }
}

int disassemble_instruction(Chunk* chunk, int offset)
{
    printf("%04d ", offset);
    int line = chunk->lines[offset];
    if (offset > 0 && line == chunk->lines[offset - 1])
    {
        printf("   | ");
    }
    else
    {
        printf("%4d ", line);
    }

    uint8_t instruction = chunk->code[offset];
    int next;
    switch (instruction)
    {
    case op_constant:
        next = constant_instruction("OP_CONSTANT", chunk, offset);
        break;
    case op_nil:
        next = simple_instruction("OP_NIL", offset);
        break;
    case op_true:
        next = simple_instruction("OP_TRUE", offset);
        break;
    case op_false:
        next = simple_instruction("OP_FALSE", offset);
        break;
    case op_equal:
        next = simple_instruction("OP_EQUAL", offset);
        break;
    case op_greater:
        next = simple_instruction("OP_GREATER", offset);
        break;
    case op_less:
        next = simple_instruction("OP_LESS", offset);
        break;
    case op_negate:
        next = simple_instruction("OP_NEGATE", offset);
        break;
    case op_add:
        next = simple_instruction("OP_ADD", offset);
        break;
    case op_subtract:
        next = simple_instruction("OP_SUBTRACT", offset);
        break;
    case op_multiply:
        next = simple_instruction("OP_MULTIPLY", offset);
        break;
    case op_divide:
        next = simple_instruction("OP_DIVIDE", offset);
        break;
    case op_not:
        next = simple_instruction("OP_NOT", offset);
        break;
    case op_return:
        next = simple_instruction("OP_RETURN", offset);
        break;
    default:
        next = unknown_instruction(instruction, offset);
        break;
    }
    return next;
}
