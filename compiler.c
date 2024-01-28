#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "chunk.h"
#include "compiler.h"
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

typedef void (*Function)();

typedef struct
{
    Function prefix;
    Function infix;
    Precedence precedence;
} Rule;

static Rule* get_rule(Token_type type);

Parser parser;
Chunk* compiling_chunk;

static Chunk* current_chunk()
{
    return compiling_chunk;
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
    emit_byte(op_return);
}

static void end_compiler()
{
    emit_return();
#ifdef DEBUG_PRINT_CODE
    if (!parser.had_error)
    {
        disassemble_chunk(current_chunk(), "code");
    }
#endif
}

static void parse(Precedence precedence)
{
    advance();
    Rule* current = get_rule(parser.previous.type);
    Function prefix = current->prefix;
    if (prefix != NULL)
    {
        prefix();
        Rule* next = get_rule(parser.current.type);
        while (precedence <= next->precedence)
        {
            advance();
            current = get_rule(parser.previous.type);
            Function infix = current->infix;
            infix();
            next = get_rule(parser.current.type);
        }
    }
    else
    {
        error("Expect expression.");
    }
}

static void expression()
{
    parse(prec_assignment);
}

static void print_statement()
{
    expression();
    consume(token_semicolon, "Expect ';' after value.");
    emit_byte(op_print);
}

static void statement()
{
    if (match(token_print))
    {
        print_statement();
    }
}

static void declaration()
{
    statement();
}

static void grouping()
{
    expression();
    consume(token_right_paren, "Expect ')' after expression.");
}

static void unary()
{
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

static void binary()
{
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

static void literal()
{
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

static void number()
{
    double value = strtod(parser.previous.start, NULL);
    emit_constant(number_value(value));
}

static void string()
{
    const char* chars = parser.previous.start + 1;
    int length = parser.previous.length - 2;
    String* string = copy_string(chars, length);
    emit_constant(object_value((Object*)string));
}

Rule rules[] =
{
    [token_left_paren] = { grouping, NULL, prec_none },
    [token_right_paren] = { NULL, NULL, prec_none },
    [token_left_brace] = { NULL, NULL, prec_none },
    [token_right_brace] = { NULL, NULL, prec_none },
    [token_comma] = { NULL, NULL, prec_none },
    [token_dot] = { NULL, NULL, prec_none },
    [token_minus] = { unary, binary, prec_term },
    [token_plus] = { NULL, binary, prec_term },
    [token_semicolon] = { NULL, NULL, prec_none },
    [token_slash] = { NULL, binary, prec_factor },
    [token_star] = { NULL, binary, prec_factor },
    [token_bang] = { unary, NULL, prec_none },
    [token_bang_equal] = { NULL, binary, prec_equality },
    [token_equal] = { NULL, NULL, prec_none },
    [token_equal_equal] = { NULL, binary, prec_equality },
    [token_greater] = { NULL, binary, prec_comparison },
    [token_greater_equal] = { NULL, binary, prec_comparison },
    [token_less] = { NULL, binary, prec_comparison },
    [token_less_equal] = { NULL, binary, prec_comparison },
    [token_identifier] = { NULL, NULL, prec_none },
    [token_string] = { string, NULL, prec_none },
    [token_number] = { number, NULL, prec_none },
    [token_and] = { NULL, NULL, prec_none },
    [token_class] = { NULL, NULL, prec_none },
    [token_else] = { NULL, NULL, prec_none },
    [token_false] = { literal, NULL, prec_none },
    [token_for] = { NULL, NULL, prec_none },
    [token_fun] = { NULL, NULL, prec_none },
    [token_if] = { NULL, NULL, prec_none },
    [token_nil] = { literal, NULL, prec_none },
    [token_or] = { NULL, NULL, prec_none },
    [token_print] = { NULL, NULL, prec_none },
    [token_return] = { NULL, NULL, prec_none },
    [token_super] = { NULL, NULL, prec_none },
    [token_this] = { NULL, NULL, prec_none },
    [token_true] = { literal, NULL, prec_none },
    [token_var] = { NULL, NULL, prec_none },
    [token_while] = { NULL, NULL, prec_none },
    [token_error] = { NULL, NULL, prec_none },
    [token_eof] = { NULL, NULL, prec_none }
};

static Rule* get_rule(Token_type type)
{
    return &rules[type];
}

bool compile(const char* source, Chunk* chunk)
{
    init_scanner(source);
    compiling_chunk = chunk;
    parser.had_error = false;
    parser.panic_mode = false;
    advance();
    while (!match(token_eof))
    {
        declaration();
    }
    end_compiler();
    return !parser.had_error;
}
