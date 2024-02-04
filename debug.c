#include <stdint.h>
#include <stdio.h>

#include "chunk.h"
#include "debug.h"
#include "object.h"
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

static int byte_instruction(const char* name, Chunk* chunk, int offset)
{
    uint8_t slot = chunk->code[offset + 1];
    printf("%-16s %4d\n", name, slot);
    return offset + 2;
}

static int jump_instruction(const char* name, int sign, Chunk* chunk, int offset)
{
    uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
    jump |= chunk->code[offset + 2];
    printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
    return offset + 3;
}

static int closure_instruction(const char* name, Chunk* chunk, int offset)
{
    offset++;
    uint8_t constant = chunk->code[offset++];
    printf("%-16s %4d ", name, constant);
    print_value(chunk->constants.values[constant]);
    printf("\n");
    Function* function = as_function(chunk->constants.values[constant]);
    for (int i = 0; i < function->upvalue_count; i++)
    {
        int is_local = chunk->code[offset++];
        int index = chunk->code[offset++];
        printf("%04d      |                   %s %d\n", offset - 2, is_local ? "local" : "upvalue",
            index);
    }
    return offset;
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
    case op_pop:
        next = simple_instruction("OP_POP", offset);
        break;
    case op_get_local:
        next = byte_instruction("OP_GET_LOCAL", chunk, offset);
        break;
    case op_set_local:
        next = byte_instruction("OP_SET_LOCAL", chunk, offset);
        break;
    case op_get_upvalue:
        next = byte_instruction("OP_GET_UPVALUE", chunk, offset);
        break;
    case op_set_upvalue:
        next = byte_instruction("OP_SET_UPVALUE", chunk, offset);
        break;
    case op_close_upvalue:
        next = simple_instruction("OP_CLOSE_UPVALUE", offset);
        break;
    case op_get_global:
        next = constant_instruction("OP_GET_GLOBAL", chunk, offset);
        break;
    case op_define_global:
        next = constant_instruction("OP_DEFINE_GLOBAL", chunk, offset);
        break;
    case op_set_global:
        next = constant_instruction("OP_SET_GLOBAL", chunk, offset);
        break;
    case op_get_property:
        next = constant_instruction("OP_GET_PROPERTY", chunk, offset);
        break;
    case op_set_property:
        next = constant_instruction("OP_SET_PROPERTY", chunk, offset);
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
    case op_print:
        next = simple_instruction("OP_PRINT", offset);
        break;
    case op_jump:
        next = jump_instruction("OP_JUMP", 1, chunk, offset);
        break;
    case op_jump_if_false:
        next = jump_instruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
        break;
    case op_loop:
        next = jump_instruction("OP_LOOP", -1, chunk, offset);
        break;
    case op_call:
        next = byte_instruction("OP_CALL", chunk, offset);
        break;
    case op_closure:
        next = closure_instruction("OP_CLOSURE", chunk, offset);
        break;
    case op_return:
        next = simple_instruction("OP_RETURN", offset);
        break;
    case op_class:
        next = constant_instruction("OP_CLASS", chunk, offset);
        break;
    default:
        next = unknown_instruction(instruction, offset);
        break;
    }
    return next;
}
