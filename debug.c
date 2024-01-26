#include <stdint.h>
#include <stdio.h>

#include "debug.h"
#include "value.h"

static int disassemble_instruction(Chunk* chunk, int offset)
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
        case op_return:
            next = simple_instruction("OP_RETURN", offset);
            break;
        default:
            printf("Unknown opcode %d\n", instruction);
            next = offset + 1;
            break;
    }
    return next;
}

static int constant_instruction(const char* name,
                                Chunk*      chunk,
                                int         offset)
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

void disassemble_chunk(Chunk* chunk, const char* name)
{
    printf("== %s ==\n", name);
    int offset = 0;
    while (offset < chunk->count)
    {
        offset = disassemble_instruction(chunk, offset);
    }
}


