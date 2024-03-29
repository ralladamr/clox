#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.h"
#include "compiler.h"
#include "memory.h"
#include "object.h"
#include "scanner.h"
#include "value.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct
{
    Token current;
    Token previous;
    bool had_error;
    bool panic_mode;
} Parser;

typedef enum
{
    prec_none,
    prec_assignment,
    prec_or,
    prec_and,
    prec_equality,
    prec_comparison,
    prec_term,
    prec_factor,
    prec_unary,
    prec_call,
    prec_primary
} Precedence;

typedef struct
{
    void (*prefix)(bool);
    void (*infix)(bool);
    Precedence precedence;
} Rule;

typedef struct
{
    Token name;
    int depth;
    bool is_captured;
} Local;

typedef struct
{
    uint8_t index;
    bool is_local;
} Upvalue_node;

typedef enum
{
    type_function,
    type_method,
    type_initializer,
    type_script
} Function_type;

typedef struct Compiler
{
    struct Compiler* enclosing;
    Function* function;
    Function_type type;
    Local locals[variables_max];
    int local_count;
    Upvalue_node upvalues[variables_max];
    int scope_depth;
} Compiler;

typedef struct Class_compiler
{
    struct Class_compiler* enclosing;
    bool has_superclass;
} Class_compiler;

Parser parser;
Compiler* current = NULL;
Class_compiler* current_class = NULL;

static void block();
static void statement();
static Rule* get_rule(Token_type type);

static Chunk* current_chunk()
{
    return &current->function->chunk;
}

static void error_at(Token* token, const char* message)
{
    if (!parser.panic_mode)
    {
        parser.panic_mode = true;
        fprintf(stderr, "[line %d] Error", token->line);
        switch (token->type)
        {
        case token_eof:
            fprintf(stderr, " at end");
            break;
        case token_error:
            break;
        default:
            fprintf(stderr, " at '%.*s'", token->length, token->start);
            break;
        }
        fprintf(stderr, ": %s\n", message);
        parser.had_error = true;
    }
}

static void error(const char* message)
{
    error_at(&parser.previous, message);
}

static void error_at_current(const char* message)
{
    error_at(&parser.current, message);
}

static void init_compiler(Compiler* compiler, Function_type type)
{
    compiler->enclosing = current;
    compiler->function = NULL;
    compiler->type = type;
    compiler->local_count = 0;
    compiler->scope_depth = 0;
    compiler->function = new_function();
    current = compiler;
    if (type != type_script)
    {
        current->function->name = copy_string(parser.previous.start, parser.previous.length);
    }

    Local* local = &current->locals[current->local_count++];
    local->depth = 0;
    local->is_captured = false;
    if (type != type_function)
    {
        local->name.start = "this";
        local->name.length = 4;
    }
    else
    {
        local->name.start = "";
        local->name.length = 0;
    }
}

static void advance()
{
    parser.previous = parser.current;
    bool valid = false;
    while (!valid)
    {
        parser.current = scan_token();
        if (parser.current.type != token_error)
        {
            valid = true;
        }
        else
        {
            error_at_current(parser.current.start);
        }
    }
}

static void consume(Token_type type, const char* message)
{
    if (parser.current.type == type)
    {
        advance();
    }
    else
    {
        error_at_current(message);
    }
}

static bool check(Token_type type)
{
    return parser.current.type == type;
}

static bool match(Token_type type)
{
    bool matched = false;
    if (check(type))
    {
        advance();
        matched = true;
    }
    return matched;
}

static uint8_t make_constant(Value value)
{
    Chunk* chunk = current_chunk();
    int constant = add_constant(chunk, value);
    uint8_t result;
    if (constant > UINT8_MAX)
    {
        error("Too many constants in one chunk.");
        result = 0;
    }
    else
    {
        result = (uint8_t)constant;
    }
    return result;
}

static void emit_byte(uint8_t byte)
{
    Chunk* chunk = current_chunk();
    write_chunk(chunk, byte, parser.previous.line);
}

static void emit_bytes(uint8_t a, uint8_t b)
{
    emit_byte(a);
    emit_byte(b);
}

static void emit_constant(Value value)
{
    emit_bytes(op_constant, make_constant(value));
}

static void emit_return()
{
    if (current->type == type_initializer)
    {
        emit_bytes(op_get_local, 0);
    }
    else
    {
        emit_byte(op_nil);
    }
    emit_byte(op_return);
}

static int emit_jump(uint8_t instruction)
{
    emit_byte(instruction);
    emit_byte(0xff);
    emit_byte(0xff);
    return current_chunk()->count - 2;
}

static void emit_loop(int loop_start)
{
    emit_byte(op_loop);
    int offset = current_chunk()->count - loop_start + 2;
    if (offset > UINT16_MAX)
    {
        error("Loop body too large.");
    }
    emit_byte((offset >> 8) & 0xff);
    emit_byte(offset & 0xff);
}

static void patch_jump(int offset)
{
    int jump = current_chunk()->count - offset - 2;
    if (jump > UINT16_MAX)
    {
        error("Too much code to jump over.");
    }
    current_chunk()->code[offset] = (jump >> 8) & 0xff;
    current_chunk()->code[offset + 1] = jump & 0xff;
}

static Function* end_compiler()
{
    emit_return();
    Function* function = current->function;

#ifdef DEBUG_PRINT_CODE
    if (!parser.had_error)
    {
        char* name = "<script>";
        if (function->name != NULL)
        {
            name = function->name->chars;
        }
        disassemble_chunk(current_chunk(), name);
    }
#endif

    current = current->enclosing;
    return function;
}

static void parse(Precedence precedence)
{
    advance();
    Rule* current = get_rule(parser.previous.type);
    void (*prefix)(bool) = current->prefix;
    if (prefix != NULL)
    {
        bool can_assign = precedence <= prec_assignment;
        prefix(can_assign);
        Rule* next = get_rule(parser.current.type);
        while (precedence <= next->precedence)
        {
            advance();
            current = get_rule(parser.previous.type);
            void (*infix)(bool) = current->infix;
            infix(can_assign);
            next = get_rule(parser.current.type);
        }
        if (can_assign && match(token_equal))
        {
            error("Invalid assignment target.");
        }
    }
    else
    {
        error("Expect expression.");
    }
}

static uint8_t identifier_constant(Token* name)
{
    String* string = copy_string(name->start, name->length);
    Value value = object_value((Object*)string);
    return make_constant(value);
}

static void add_local(Token name)
{
    if (current->local_count < variables_max)
    {
        Local* local = &current->locals[current->local_count];
        current->local_count++;
        local->name = name;
        local->depth = -1;
        local->is_captured = false;
    }
    else
    {
        error("Too many local variables in function.");
    }
}

static bool identifiers_equal(Token* a, Token* b)
{
    return a->length == b->length && memcmp(a->start, b->start, a->length) == 0;
}

static void declare_variable()
{
    if (current->scope_depth > 0)
    {
        Token* name = &parser.previous;
        int i = current->local_count - 1;
        bool in_scope = true;
        while (in_scope && i >= 0)
        {
            Local* local = &current->locals[i];
            if (local->depth != -1 && local->depth < current->scope_depth)
            {
                in_scope = false;
            }
            if (identifiers_equal(name, &local->name))
            {
                error("Already a variable with this name in this scope.");
            }
            i--;
        }
        add_local(*name);
    }
}

static uint8_t parse_variable(const char* message)
{
    consume(token_identifier, message);
    declare_variable();
    return current->scope_depth > 0 ? 0 : identifier_constant(&parser.previous);
}

static void mark_initialized()
{
    if (current->scope_depth != 0)
    {
        current->locals[current->local_count - 1].depth = current->scope_depth;
    }
}

static void define_variable(uint8_t global)
{
    if (current->scope_depth > 0)
    {
        mark_initialized();
    }
    else if (current->scope_depth == 0)
    {
        emit_bytes(op_define_global, global);
    }
}

static void expression()
{
    parse(prec_assignment);
}

static void expression_statement()
{
    expression();
    consume(token_semicolon, "Expect ';' after expression.");
    emit_byte(op_pop);
}

static void print_statement()
{
    expression();
    consume(token_semicolon, "Expect ';' after value.");
    emit_byte(op_print);
}

static void synchronize()
{
    parser.panic_mode = false;
    bool done = false;
    while (parser.current.type != token_eof || done)
    {
        if (parser.previous.type == token_semicolon)
        {
            done = true;
        }
        switch (parser.current.type)
        {
        case token_class:
        case token_fun:
        case token_var:
        case token_for:
        case token_if:
        case token_while:
        case token_print:
        case token_return:
            done = true;
            break;
        default:
            break;
        }
        if (!done)
        {
            advance();
        }
    }
}

static void var_declaration()
{
    uint8_t global = parse_variable("Expect variable name.");
    if (match(token_equal))
    {
        expression();
    }
    else
    {
        emit_byte(op_nil);
    }
    consume(token_semicolon, "Expect ';' after variable declaration.");
    define_variable(global);
}

static void begin_scope()
{
    current->scope_depth++;
}

static void end_scope()
{
    current->scope_depth--;
    while (current->local_count > 0
        && current->locals[current->local_count - 1].depth > current->scope_depth)
    {
        if (current->locals[current->local_count - 1].is_captured)
        {
            emit_byte(op_close_upvalue);
        }
        else
        {
            emit_byte(op_pop);
        }
        current->local_count--;
    }
}

static void function(Function_type type)
{
    Compiler compiler;
    init_compiler(&compiler, type);
    begin_scope();
    consume(token_left_paren, "Expect '(' after function name.");
    if (!check(token_right_paren))
    {
        do
        {
            current->function->arity++;
            if (current->function->arity > 255)
            {
                error_at_current("Can't have more than 255 parameters.");
            }
            uint8_t constant = parse_variable("Expect parameter name.");
            define_variable(constant);
        }
        while (match(token_comma));
    }
    consume(token_right_paren, "Expect ')' after parameters.");
    consume(token_left_brace, "Expect '{' before function body.");
    block();
    Function* function = end_compiler();
    emit_bytes(op_closure, make_constant(object_value((Object*)function)));
    for (int i = 0; i < function->upvalue_count; i++)
    {
        emit_byte(compiler.upvalues[i].is_local ? 1 : 0);
        emit_byte(compiler.upvalues[i].index);
    }
}

static void method()
{
    consume(token_identifier, "Expect method name.");
    uint8_t constant = identifier_constant(&parser.previous);
    Function_type type = type_method;
    if (parser.previous.length == 4 && memcmp(parser.previous.start, "init", 4) == 0)
    {
        type = type_initializer;
    }
    function(type);
    emit_bytes(op_method, constant);
}

static int resolve_local(Compiler* compiler, Token* name)
{
    bool found = false;
    int i = compiler->local_count - 1;
    while (!found && i >= 0)
    {
        Local* local = &compiler->locals[i];
        if (identifiers_equal(name, &local->name))
        {
            if (local->depth == -1)
            {
                error("Can't read local variable in its own initializer.");
            }
            found = true;
        }
        else
        {
            i--;
        }
    }
    return found ? i : -1;
}

static int add_upvalue(Compiler* compiler, uint8_t index, bool is_local)
{
    int count = compiler->function->upvalue_count;
    int result = -1;
    for (int i = 0; i < count; i++)
    {
        Upvalue_node* upvalue = &compiler->upvalues[i];
        if (upvalue->index == index && upvalue->is_local == is_local)
        {
            result = i;
        }
    }
    if (result == -1)
    {
        if (count == variables_max)
        {
            error("Too many closure variables in function.");
            result = 0;
        }
        else
        {
            compiler->upvalues[count].is_local = is_local;
            compiler->upvalues[count].index = index;
            result = compiler->function->upvalue_count++;
        }
    }
    return result;
}

static int resolve_upvalue(Compiler* compiler, Token* name)
{
    int result = -1;
    if (compiler->enclosing != NULL)
    {
        int local = resolve_local(compiler->enclosing, name);
        if (local != -1)
        {
            compiler->enclosing->locals[local].is_captured = true;
            result = add_upvalue(compiler, (uint8_t)local, true);
        }
        else
        {
            int upvalue = resolve_upvalue(compiler->enclosing, name);
            if (upvalue != -1)
            {
                result = add_upvalue(compiler, (uint8_t)upvalue, false);
            }
        }
    }
    return result;
}

static void named_variable(Token name, bool can_assign)
{
    uint8_t get_op;
    uint8_t set_op;
    int arg = resolve_local(current, &name);
    if (arg != -1)
    {
        get_op = op_get_local;
        set_op = op_set_local;
    }
    else
    {
        arg = resolve_upvalue(current, &name);
        if (arg != -1)
        {
            get_op = op_get_upvalue;
            set_op = op_set_upvalue;
        }
        else
        {
            arg = identifier_constant(&name);
            get_op = op_get_global;
            set_op = op_set_global;
        }
    }

    if (can_assign && match(token_equal))
    {
        expression();
        emit_bytes(set_op, (uint8_t)arg);
    }
    else
    {
        emit_bytes(get_op, (uint8_t)arg);
    }
}

static void variable(bool can_assign)
{
    named_variable(parser.previous, can_assign);
}

static Token synthetic_token(const char* text)
{
    Token token = {.start = text, .length = (int)strlen(text)};
    return token;
}

static void class_declaration()
{
    consume(token_identifier, "Expect class name.");
    Token class_name = parser.previous;
    uint8_t name_constant = identifier_constant(&parser.previous);
    declare_variable();
    emit_bytes(op_class, name_constant);
    define_variable(name_constant);
    Class_compiler class_compiler = {.enclosing = current_class, .has_superclass = false};
    current_class = &class_compiler;
    if (match(token_less))
    {
        consume(token_identifier, "Expect superclass name.");
        variable(false);
        if (identifiers_equal(&class_name, &parser.previous))
        {
            error("A class can't inherit from itself");
        }
        begin_scope();
        add_local(synthetic_token("super"));
        define_variable(0);
        named_variable(class_name, false);
        emit_byte(op_inherit);
        class_compiler.has_superclass = true;
    }
    named_variable(class_name, false);
    consume(token_left_brace, "Expect '{' before class body.");
    while (!check(token_right_brace) && !check(token_eof))
    {
        method();
    }
    consume(token_right_brace, "Expect '}' after class body.");
    emit_byte(op_pop);
    if (class_compiler.has_superclass)
    {
        end_scope();
    }
    current_class = current_class->enclosing;
}

static void fun_declaration()
{
    uint8_t global = parse_variable("Expect function name.");
    mark_initialized();
    function(type_function);
    define_variable(global);
}

static void for_statement()
{
    begin_scope();
    consume(token_left_paren, "Expect '(' after 'for'.");
    if (match(token_semicolon))
    {
        // No initializer.
    }
    else if (match(token_var))
    {
        var_declaration();
    }
    else
    {
        expression_statement();
    }

    int loop_start = current_chunk()->count;
    int exit_jump = -1;
    if (!match(token_semicolon))
    {
        expression();
        consume(token_semicolon, "Expect ';' after loop condition.");
        exit_jump = emit_jump(op_jump_if_false);
        emit_byte(op_pop);
    }

    if (!match(token_right_paren))
    {
        int body_jump = emit_jump(op_jump);
        int increment_start = current_chunk()->count;
        expression();
        emit_byte(op_pop);
        consume(token_right_paren, "Expect ')' after for clauses.");
        emit_loop(loop_start);
        loop_start = increment_start;
        patch_jump(body_jump);
    }

    statement();
    emit_loop(loop_start);
    if (exit_jump != -1)
    {
        patch_jump(exit_jump);
        emit_byte(op_pop);
    }
    end_scope();
}

static void if_statement()
{
    consume(token_left_paren, "Expect '(' after 'if'.");
    expression();
    consume(token_right_paren, "Expect ')' after condition.");
    int then_jump = emit_jump(op_jump_if_false);
    emit_byte(op_pop);
    statement();
    int else_jump = emit_jump(op_jump);
    patch_jump(then_jump);
    emit_byte(op_pop);
    if (match(token_else))
    {
        statement();
    }
    patch_jump(else_jump);
}

static void return_statement()
{
    if (current->type == type_script)
    {
        error("Can't return from top-level code.");
    }

    if (match(token_semicolon))
    {
        emit_return();
    }
    else
    {
        if (current->type == type_initializer)
        {
            error("Can't return a value from an initializer.");
        }
        expression();
        consume(token_semicolon, "Expect ';' after return value.");
        emit_byte(op_return);
    }
}

static void while_statement()
{
    int loop_start = current_chunk()->count;
    consume(token_left_paren, "Expect '(' after 'while'.");
    expression();
    consume(token_right_paren, "Expect ')' after condition.");
    int exit_jump = emit_jump(op_jump_if_false);
    emit_byte(op_pop);
    statement();
    emit_loop(loop_start);
    patch_jump(exit_jump);
    emit_byte(op_pop);
}

static void statement()
{
    if (match(token_print))
    {
        print_statement();
    }
    else if (match(token_for))
    {
        for_statement();
    }
    else if (match(token_if))
    {
        if_statement();
    }
    else if (match(token_return))
    {
        return_statement();
    }
    else if (match(token_while))
    {
        while_statement();
    }
    else if (match(token_left_brace))
    {
        begin_scope();
        block();
        end_scope();
    }
    else
    {
        expression_statement();
    }
}

static void declaration()
{
    if (match(token_class))
    {
        class_declaration();
    }
    else if (match(token_fun))
    {
        fun_declaration();
    }
    else if (match(token_var))
    {
        var_declaration();
    }
    else
    {
        statement();
    }

    if (parser.panic_mode)
    {
        synchronize();
    }
}

static void block()
{
    while (!check(token_right_brace) && !check(token_eof))
    {
        declaration();
    }
    consume(token_right_brace, "Expect '}' after block.");
}

static void grouping(bool can_assign)
{
    (void)can_assign;
    expression();
    consume(token_right_paren, "Expect ')' after expression.");
}

static void unary(bool can_assign)
{
    (void)can_assign;
    Token_type operator = parser.previous.type;
    parse(prec_unary);
    switch (operator)
    {
    case token_bang:
        emit_byte(op_not);
        break;
    case token_minus:
        emit_byte(op_negate);
        break;
    default:
        break;
    }
}

static void binary(bool can_assign)
{
    (void)can_assign;
    Token_type operator = parser.previous.type;
    Rule* rule = get_rule(operator);
    Precedence precedence = (Precedence)(rule->precedence + 1);
    parse(precedence);
    switch (operator)
    {
    case token_bang_equal:
        emit_bytes(op_equal, op_not);
        break;
    case token_equal_equal:
        emit_byte(op_equal);
        break;
    case token_greater:
        emit_byte(op_greater);
        break;
    case token_greater_equal:
        emit_bytes(op_less, op_not);
        break;
    case token_less:
        emit_byte(op_less);
        break;
    case token_less_equal:
        emit_bytes(op_greater, op_not);
        break;
    case token_plus:
        emit_byte(op_add);
        break;
    case token_minus:
        emit_byte(op_subtract);
        break;
    case token_star:
        emit_byte(op_multiply);
        break;
    case token_slash:
        emit_byte(op_divide);
        break;
    default:
        break;
    }
}

static uint8_t argument_list()
{
    uint8_t arg_count = 0;
    if (!check(token_right_paren))
    {
        do
        {
            expression();
            if (arg_count == 255)
            {
                error("Can't have more than 255 arguments.");
            }
            arg_count++;
        }
        while (match(token_comma));
    }
    consume(token_right_paren, "Expect ')' after arguments.");
    return arg_count;
}

static void call(bool can_assign)
{
    (void)can_assign;
    uint8_t arg_count = argument_list();
    emit_bytes(op_call, arg_count);
}

static void dot(bool can_assign)
{
    consume(token_identifier, "Expect property name after '.'.");
    uint8_t name = identifier_constant(&parser.previous);
    if (can_assign && match(token_equal))
    {
        expression();
        emit_bytes(op_set_property, name);
    }
    else if (match(token_left_paren))
    {
        uint8_t arg_count = argument_list();
        emit_bytes(op_invoke, name);
        emit_byte(arg_count);
    }
    else
    {
        emit_bytes(op_get_property, name);
    }
}

static void literal(bool can_assign)
{
    (void)can_assign;
    Token_type operator = parser.previous.type;
    switch (operator)
    {
    case token_false:
        emit_byte(op_false);
        break;
    case token_nil:
        emit_byte(op_nil);
        break;
    case token_true:
        emit_byte(op_true);
        break;
    default:
        break;
    }
}

static void number(bool can_assign)
{
    (void)can_assign;
    double value = strtod(parser.previous.start, NULL);
    emit_constant(number_value(value));
}

static void string(bool can_assign)
{
    (void)can_assign;
    const char* chars = parser.previous.start + 1;
    int length = parser.previous.length - 2;
    String* string = copy_string(chars, length);
    emit_constant(object_value((Object*)string));
}

static void and_(bool can_assign)
{
    (void)can_assign;
    int end_jump = emit_jump(op_jump_if_false);
    emit_byte(op_pop);
    parse(prec_and);
    patch_jump(end_jump);
}

static void or_(bool can_assign)
{
    (void)can_assign;
    int else_jump = emit_jump(op_jump_if_false);
    int end_jump = emit_jump(op_jump);
    patch_jump(else_jump);
    emit_byte(op_pop);
    parse(prec_or);
    patch_jump(end_jump);
}

static void this_(bool can_assign)
{
    (void)can_assign;
    if (current_class == NULL)
    {
        error("Can't use 'this' outside of a class.");
    }
    else
    {
        variable(false);
    }
}

static void super_(bool can_assign)
{
    (void)can_assign;
    if (current_class == NULL)
    {
        error("Can't user 'super' outside of a class.");
    }
    else if (!current_class->has_superclass)
    {
        error("Can't use 'super' in a class with no superclass.");
    }
    consume(token_dot, "Expect '.' after 'super'.");
    consume(token_identifier, "Expect superclass after method name.");
    uint8_t name = identifier_constant(&parser.previous);
    named_variable(synthetic_token("this"), false);
    if (match(token_left_paren))
    {
        uint8_t arg_count = argument_list();
        named_variable(synthetic_token("super"), false);
        emit_bytes(op_super_invoke, name);
        emit_byte(arg_count);
    }
    else
    {
        named_variable(synthetic_token("super"), false);
        emit_bytes(op_get_super, name);
    }
}

Rule rules[] =
{
    [token_left_paren] = {grouping, call, prec_call},
    [token_right_paren] = {NULL, NULL, prec_none},
    [token_left_brace] = {NULL, NULL, prec_none},
    [token_right_brace] = {NULL, NULL, prec_none},
    [token_comma] = {NULL, NULL, prec_none},
    [token_dot] = {NULL, dot, prec_call},
    [token_minus] = {unary, binary, prec_term},
    [token_plus] = {NULL, binary, prec_term},
    [token_semicolon] = {NULL, NULL, prec_none},
    [token_slash] = {NULL, binary, prec_factor},
    [token_star] = {NULL, binary, prec_factor},
    [token_bang] = {unary, NULL, prec_none},
    [token_bang_equal] = {NULL, binary, prec_equality},
    [token_equal] = {NULL, NULL, prec_none},
    [token_equal_equal] = {NULL, binary, prec_equality},
    [token_greater] = {NULL, binary, prec_comparison},
    [token_greater_equal] = {NULL, binary, prec_comparison},
    [token_less] = {NULL, binary, prec_comparison},
    [token_less_equal] = {NULL, binary, prec_comparison},
    [token_identifier] = {variable, NULL, prec_none},
    [token_string] = {string, NULL, prec_none},
    [token_number] = {number, NULL, prec_none},
    [token_and] = {NULL, and_, prec_and},
    [token_class] = {NULL, NULL, prec_none},
    [token_else] = {NULL, NULL, prec_none},
    [token_false] = {literal, NULL, prec_none},
    [token_for] = {NULL, NULL, prec_none},
    [token_fun] = {NULL, NULL, prec_none},
    [token_if] = {NULL, NULL, prec_none},
    [token_nil] = {literal, NULL, prec_none},
    [token_or] = {NULL, or_, prec_or},
    [token_print] = {NULL, NULL, prec_none},
    [token_return] = {NULL, NULL, prec_none},
    [token_super] = {super_, NULL, prec_none},
    [token_this] = {this_, NULL, prec_none},
    [token_true] = {literal, NULL, prec_none},
    [token_var] = {NULL, NULL, prec_none},
    [token_while] = {NULL, NULL, prec_none},
    [token_error] = {NULL, NULL, prec_none},
    [token_eof] = {NULL, NULL, prec_none}
};

static Rule* get_rule(Token_type type)
{
    return &rules[type];
}

Function* compile(const char* source)
{
    init_scanner(source);
    Compiler compiler;
    init_compiler(&compiler, type_script);
    parser.had_error = false;
    parser.panic_mode = false;
    advance();
    while (!match(token_eof))
    {
        declaration();
    }
    Function* function = end_compiler();
    return parser.had_error ? NULL : function;
}

void mark_compiler_roots()
{
    Compiler* compiler = current;
    while (compiler != NULL)
    {
        mark_object((Object*)compiler->function);
        compiler = compiler->enclosing;
    }
}
