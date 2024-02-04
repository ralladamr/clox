#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

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

static Value clock_native(int arg_count, Value* args)
{
    (void)arg_count;
    (void)args;
    return number_value((double)clock() / CLOCKS_PER_SEC);
}

static inline uint8_t read_byte(Call_frame* frame)
{
    return *frame->ip++;
}

static inline Value read_constant(Call_frame* frame)
{
    uint8_t byte = read_byte(frame);
    return frame->closure->function->chunk.constants.values[byte];
}

static inline uint16_t read_short(Call_frame* frame)
{
    frame->ip += 2;
    return (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]);
}

static inline String* read_string(Call_frame* frame)
{
    return as_string(read_constant(frame));
}

static Value peek(int distance)
{
    return vm.stack_top[-1 - distance];
}

static void runtime_error(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = vm.frame_count - 1; i >= 0; i--)
    {
        Call_frame* frame = &vm.frames[i];
        Function* function = frame->closure->function;
        size_t instruction = frame->ip - function->chunk.code - 1;
        fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);
        if (function->name == NULL)
        {
            fprintf(stderr, "script\n");
        }
        else
        {
            fprintf(stderr, "%s()\n", function->name->chars);
        }
    }
}

static void define_native(const char* name, Value(*function)(int, Value*))
{
    push(object_value((Object*)copy_string(name, (int)strlen(name))));
    push(object_value((Object*)new_native(function)));
    table_set(&vm.globals, as_string(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}

static bool call(Closure* closure, int arg_count)
{
    bool result = false;
    if (arg_count != closure->function->arity)
    {
        runtime_error("Expected %d arguments but got %d.",
            closure->function->arity, arg_count);
    }
    else if (vm.frame_count == frames_max)
    {
        runtime_error("Stack overflow.");
    }
    else
    {
        Call_frame* frame = &vm.frames[vm.frame_count++];
        frame->closure = closure;
        frame->ip = closure->function->chunk.code;
        frame->slots = vm.stack_top - arg_count - 1;
        return true;
    }
    return result;
}

static bool call_value(Value callee, int arg_count)
{
    bool result = false;
    if (is_object(callee))
    {
        switch (object_type(callee))
        {
        case obj_class:
        {
            Class* class = as_class(callee);
            vm.stack_top[-arg_count - 1] = object_value((Object*)new_instance(class));
            result = true;
            break;
        }
        case obj_closure:
            result = call(as_closure(callee), arg_count);
            break;
        case obj_native:
        {
            Value(*native)(int, Value*) = as_native(callee);
            Value val = native(arg_count, vm.stack_top - arg_count);
            vm.stack_top -= arg_count + 1;
            push(val);
            result = true;
            break;
        }
        default:
            runtime_error("Can only call functions and classes.");
            break;
        }
    }
    return result;
}

static Upvalue* capture_upvalue(Value* local)
{
    Upvalue* previous = NULL;
    Upvalue* upvalue = vm.open_upvalues;
    while (upvalue != NULL && upvalue->location > local)
    {
        previous = upvalue;
        upvalue = upvalue->next;
    }
    Upvalue* result;
    if (upvalue != NULL && upvalue->location == local)
    {
        result = upvalue;
    }
    else
    {
        result = new_upvalue(local);
        result->next = upvalue;
        if (previous == NULL)
        {
            vm.open_upvalues = result;
        }
        else
        {
            previous->next = result;
        }
    }
    return result;
}

static void close_upvalues(Value* last)
{
    while (vm.open_upvalues != NULL && vm.open_upvalues->location >= last)
    {
        Upvalue* upvalue = vm.open_upvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm.open_upvalues = upvalue->next;
    }
}

static bool is_falsey(Value value)
{
    return is_nil(value) || (is_bool(value) && !as_bool(value));
}

static void reset_stack()
{
    vm.stack_top = vm.stack;
    vm.frame_count = 0;
    vm.open_upvalues = NULL;
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
        else
        {
            double val = -as_number(pop());
            push(number_value(val));
        }
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
            break;
        default:
            break;
        }
    }
    return result;
}

static void concatenate()
{
    String* b = as_string(peek(0));
    String* a = as_string(peek(1));
    int length = a->length + b->length;
    char* chars = allocate_char(length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';
    String* result = take_string(chars, length);
    pop();
    pop();
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
    Call_frame* frame = &vm.frames[vm.frame_count - 1];
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
        Chunk* chunk = &frame->closure->function->chunk;
        disassemble_instruction(chunk, (int)(frame->ip - chunk->code));
#endif
        uint8_t instruction = read_byte(frame);
        switch (instruction)
        {
        case op_constant:
            push(read_constant(frame));
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
            uint8_t slot = read_byte(frame);
            push(frame->slots[slot]);
            break;
        }
        case op_set_local:
        {
            uint8_t slot = read_byte(frame);
            frame->slots[slot] = peek(0);
            break;
        }
        case op_get_upvalue:
        {
            uint8_t slot = read_byte(frame);
            push(*frame->closure->upvalues[slot]->location);
            break;
        }
        case op_set_upvalue:
        {
            uint8_t slot = read_byte(frame);
            *frame->closure->upvalues[slot]->location = peek(0);
            break;
        }
        case op_close_upvalue:
            close_upvalues(vm.stack_top - 1);
            pop();
            break;
        case op_get_global:
        {
            String* name = read_string(frame);
            Value value;
            if (!table_get(&vm.globals, name, &value))
            {
                runtime_error("Undefined variable '%s'.", name->chars);
                result = interpret_runtime_error;
            }
            else
            {
                push(value);
            }
            break;
        }
        case op_define_global:
        {
            String* name = read_string(frame);
            table_set(&vm.globals, name, peek(0));
            pop();
            break;
        }
        case op_set_global:
        {
            String* name = read_string(frame);
            bool is_new = table_set(&vm.globals, name, peek(0));
            if (is_new)
            {
                table_delete(&vm.globals, name);
                runtime_error("Undefined variable '%s'.", name->chars);
                result = interpret_runtime_error;
            }
            break;
        }
        case op_get_property:
        {
            if (is_instance(peek(0)))
            {
                Instance* instance = as_instance(peek(0));
                String* name = read_string(frame);
                Value value;
                if (table_get(&instance->fields, name, &value))
                {
                    pop();
                    push(value);
                }
                else
                {
                    runtime_error("Undefined property '%s'.", name->chars);
                    result = interpret_runtime_error;
                }                
            }
            else
            {
                runtime_error("Only instances have properties.");
                result = interpret_runtime_error;
            }
            break;
        }
        case op_set_property:
        {
            if (is_instance(peek(1)))
            {
                Instance* instance = as_instance(peek(1));
                table_set(&instance->fields, read_string(frame), peek(0));
                Value value = pop();
                pop();
                push(value);
            }
            else
            {
                runtime_error("Only instances have fields.");
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
            uint16_t offset = read_short(frame);
            frame->ip += offset;
            break;
        }
        case op_jump_if_false:
        {
            uint16_t offset = read_short(frame);
            if (is_falsey(peek(0)))
            {
                frame->ip += offset;
            }
            break;
        }
        case op_loop:
        {
            uint16_t offset = read_short(frame);
            frame->ip -= offset;
            break;
        }
        case op_call:
        {
            int arg_count = read_byte(frame);
            if (!call_value(peek(arg_count), arg_count))
            {
                result = interpret_runtime_error;
            }
            else
            {
                frame = &vm.frames[vm.frame_count - 1];
            }
            break;
        }
        case op_closure:
        {
            Function* function = as_function(read_constant(frame));
            Closure* closure = new_closure(function);
            push(object_value((Object*)closure));
            for (int i = 0; i < closure->upvalue_count; i++)
            {
                uint8_t is_local = read_byte(frame);
                uint8_t index = read_byte(frame);
                if (is_local)
                {
                    closure->upvalues[i] = capture_upvalue(frame->slots + index);
                }
                else
                {
                    closure->upvalues[i] = frame->closure->upvalues[index];
                }
            }
            break;
        }
        case op_return:
        {
            Value val = pop();
            close_upvalues(frame->slots);
            vm.frame_count--;
            if (vm.frame_count == 0)
            {
                pop();
                result = interpret_ok;
            }
            else
            {
                vm.stack_top = frame->slots;
                push(val);
                frame = &vm.frames[vm.frame_count - 1];
            }
            break;
        }
        case op_class:
            push(object_value((Object*)new_class(read_string(frame))));
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
    vm.bytes_allocated = 0;
    vm.next_GC = 1024ull * 1024ull;
    init_table(&vm.globals);
    init_table(&vm.strings);
    vm.gray_count = 0;
    vm.gray_capacity = 0;
    vm.gray_stack = NULL;
    define_native("clock", clock_native);
}

void free_VM()
{
    free_table(&vm.globals);
    free_table(&vm.strings);
    free_objects();
}

Interpret_result interpret(const char* source)
{
    Function* function = compile(source);
    Interpret_result result;
    if (function == NULL)
    {
        result = interpret_compile_error;
    }
    else
    {
        push(object_value((Object*)function));
        Closure* closure = new_closure(function);
        pop();
        push(object_value((Object*)closure));
        call(closure, 0);
        result = run();
    }
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
