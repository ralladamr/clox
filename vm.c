#include <stdio.h>

#include "chunk.h"
#include "debug.h"
#include "value.h"
#include "vm.h"

VM vm;

static inline uint8_t read_byte()
{
    return *vm.ip++;
}

static inline Value read_constant()
{
    uint8_t byte = read_byte();
    return vm.chunk->constants.values[byte];
}

static Interpret_result run()
{
    Interpret_result result = interpret_continue;
    while (result == interpret_continue)
    {
#ifdef DEBUG_TRACE_EXECUTION
        disassemble_instruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif

        uint8_t instruction = read_byte();
        switch (instruction)
        {
            case op_constant:
                Value constant = read_constant();
                print_value(constant);
                printf("\n");
                break;
            case op_return:
                result = interpret_ok;
                break;
            default:
                break;
        }
    }
    return result;
}

void init_VM()
{

}

void free_VM()
{

}

Interpret_result interpret(Chunk* chunk)
{   
    vm.chunk = chunk;
    vm.ip = vm.chunk->code;
    return run();
}
