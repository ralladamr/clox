#include <stdint.h>
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

static void unary_op(Op_code op)
{
    Value a = pop();
    switch (op)
    {
        case op_negate:
            push(-a);
            break;
        default:
            break;
    }
}

static void binary_op(Op_code op)
{
    Value b = pop();
    Value a = pop();
    switch (op)
    {
        case op_add:
            push(a + b);
            break;
        case op_subtract:
            push(a - b);
            break;
        case op_multiply:
            push(a * b);
            break;
        case op_divide:
            push(a / b);
            break;
        default:
            break;
    }
}

static Interpret_result run()
{
    Interpret_result result = interpret_continue;
    while (result == interpret_continue)
    {
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (Value* slot = vm.stack; slot < vm.stack_top; slot++)
        {
            printf("[ ");
            print_value(*slot);
            printf(" ]");
        }
        printf("\n");
        disassemble_instruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif

        uint8_t instruction = read_byte();
        switch (instruction)
        {
            case op_constant:
                Value constant = read_constant();
                push(constant);
                break;
            case op_negate:
                unary_op(instruction);
                break;
            case op_add:
            case op_subtract:
            case op_multiply:
            case op_divide:
                binary_op(instruction);
                break;
            case op_return:
                print_value(pop());
                printf("\n");
                result = interpret_ok;
                break;
            default:
                break;
        }
    }
    return result;
}

static void reset_stack()
{
    vm.stack_top = vm.stack;
}

void init_VM()
{
    reset_stack();
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

void push(Value value)
{
    *vm.stack_top = value;
    vm.stack_top++;
}

Value pop()
{
    vm.stack_top--;
    return *vm.stack_top;
}
