#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "chunk.h"
#include "compiler.h"
#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

#ifdef DEBUG_TRACE_EXECUTION
#include "debug.h"
#endif

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

static inline uint16_t read_short()
{
    vm.ip += 2;
    return (uint16_t)((vm.ip[-2] << 8) | vm.ip[-1]);
}

static inline String* read_string()
{
    return as_string(read_constant());
}

static Value peek(int distance)
{
    return vm.stack_top[-1 - distance];
}

static bool is_falsey(Value value)
{
    return is_nil(value) || (is_bool(value) && !as_bool(value));
}

static void reset_stack()
{
    vm.stack_top = vm.stack;
}

static void runtime_error(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = vm.ip - vm.chunk->code - 1;
    int line = vm.chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);
    reset_stack();
}

static Interpret_result unary_op(Op_code op)
{
    Interpret_result result = interpret_continue;
    switch (op)
    {
    case op_negate:
        if (!is_number(peek(0)))
        {
            runtime_error("Operand must be a number.");
            result = interpret_runtime_error;
        }
        double val = -as_number(pop());
        push(number_value(val));
        break;
    default:
        break;
    }
    return result;
}

static Interpret_result binary_op_number(Op_code op)
{
    Interpret_result result = interpret_continue;
    if (!is_number(peek(0)) || !is_number(peek(1)))
    {
        runtime_error("Operands must be numbers.");
        result = interpret_runtime_error;
    }
    else
    {
        double b = as_number(pop());
        double a = as_number(pop());
        switch (op)
        {
        case op_add:
            push(number_value(a + b));
            break;
        case op_subtract:
            push(number_value(a - b));
            break;
        case op_multiply:
            push(number_value(a * b));
            break;
        case op_divide:
            push(number_value(a / b));
            break;
        case op_greater:
            push(bool_value(a > b));
            break;
        case op_less:
            push(bool_value(a < b));
        default:
            break;
        }
    }
    return result;
}

static void concatenate()
{
    String* b = as_string(pop());
    String* a = as_string(pop());
    int length = a->length + b->length;
    char* chars = allocate_char(length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';
    String* result = take_string(chars, length);
    push(object_value((Object*)result));
}

static Interpret_result add()
{
    Interpret_result result = interpret_continue;
    if (is_string(peek(0)) && is_string(peek(1)))
    {
        concatenate();
    }
    else if (!is_number(peek(0)) || !is_number(peek(1)))
    {
        runtime_error("Operands must be two numbers or two strings.");
        result = interpret_runtime_error;
    }
    else
    {
        result = binary_op_number(op_add);
    }
    return result;
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
            push(read_constant());
            break;
        case op_nil:
            push(nil_value());
            break;
        case op_true:
            push(bool_value(true));
            break;
        case op_false:
            push(bool_value(false));
            break;
        case op_pop:
            pop();
            break;
        case op_get_local:
            {
                uint8_t slot = read_byte();
                push(vm.stack[slot]);
                break;
            }
        case op_set_local:
            {
                uint8_t slot = read_byte();
                vm.stack[slot] = peek(0);
                break;
            }
        case op_get_global:
            {
                String* name = read_string();
                Value value;
                if (!table_get(&vm.globals, name, &value))
                {
                    runtime_error("Undefined variable '%s'.", name->chars);
                    result = interpret_runtime_error;
                }
                push(value);
                break;
            }
        case op_define_global:
            {
                String* name = read_string();
                table_set(&vm.globals, name, peek(0));
                pop();
                break;
            }
        case op_set_global:
            {
                String* name = read_string();
                bool is_new = table_set(&vm.globals, name, peek(0));
                if (is_new)
                {
                    table_delete(&vm.globals, name);
                    runtime_error("Undefined variable '%s'.", name->chars);
                    result = interpret_runtime_error;
                }
                break;
            }
        case op_equal:
            {
                Value b = pop();
                Value a = pop();
                push(bool_value(values_equal(a, b)));
                break;
            }
        case op_negate:
            result = unary_op(instruction);
            break;
        case op_add:
            result = add();
            break;
        case op_greater:
        case op_less:
        case op_subtract:
        case op_multiply:
        case op_divide:
            result = binary_op_number(instruction);
            break;
        case op_not:
            push(bool_value(is_falsey(pop())));
            break;
        case op_print:
            print_value(pop());
            printf("\n");
            break;
        case op_jump:
            {
                uint16_t offset = read_short();
                vm.ip += offset;
                break;
            }
        case op_jump_if_false:
            {
                uint16_t offset = read_short();
                if (is_falsey(peek(0)))
                {
                    vm.ip += offset;
                }
                break;
            }
        case op_loop:
            {
                uint16_t offset = read_short();
                vm.ip -= offset;
                break;
            }
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
    reset_stack();
    vm.objects = NULL;
    init_table(&vm.globals);
    init_table(&vm.strings);
}

void free_VM()
{
    free_table(&vm.globals);
    free_table(&vm.strings);
    free_objects();
}

Interpret_result interpret(const char* source)
{
    Chunk chunk;
    init_chunk(&chunk);
    bool compiled = compile(source, &chunk);
    Interpret_result result;
    if (!compiled)
    {
        result = interpret_compile_error;
    }
    else
    {
        vm.chunk = &chunk;
        vm.ip = vm.chunk->code;
        result = run();
    }

    free_chunk(&chunk);
    return result;
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
