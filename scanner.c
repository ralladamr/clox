#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "scanner.h"

typedef struct
{
    const char* start;
    const char* current;
    int         line;
} Scanner;

Scanner scanner;

static inline bool at_end()
{
    return *scanner.current == '\0';
}

static inline char advance()
{
    scanner.current++;
    return scanner.current[-1];
}

static inline char peek()
{
    return *scanner.current;
}

static inline char peek_next()
{
    return at_end() ? '\0' : scanner.current[1];
}

static inline ptrdiff_t position()
{
    return scanner.current - scanner.start;
}

static inline char is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static char is_alpha(char c)
{
    bool lower = c >= 'a' && c <= 'z';
    bool upper = c >= 'A' && c <= 'Z';
    bool under = c == '_';
    return lower || upper || under;
}


static bool match(char c)
{
    bool result;
    if (at_end() || c != peek())
    {
        result = false;
    }
    else
    {
        scanner.current++;
        result = true;
    }
    return result;
}

static Token make(Token_type type)
{
    int length = (int)position();
    Token token = { type, scanner.start, length, scanner.line };
    return token;
}

static Token make_error(const char* message)
{
    int length = (int)strlen(message);
    Token token = { token_eof, scanner.start, length, scanner.line };
    return token;
}

static void skip_whitespace()
{
    bool cont = true;
    while (cont)
    {
        char c = peek();
        switch (c)
        {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                scanner.line++;
                advance();
                break;
            case '/':
                if (peek_next() == '/')
                {
                    while (peek() != '\n' && !at_end())
                    {
                        advance();
                    }
                }
                else
                {
                    cont = false;
                }
                break;
            default:
                cont = false;
                break;
        }
    }
}

static Token_type check_keyword(int         start,
                                int         length,
                                const char* rest,
                                Token_type  expected)
{
    Token_type type = token_identifier;
    if (position() == (size_t)start + length)
    {
        int result = memcmp(scanner.start + start, rest, (size_t)length);
        if (result == 0)
        {
            type = expected;
        }
    }
    return type;
}

static Token_type identifier_type()
{
    Token_type type = token_identifier;
    switch (scanner.start[0])
    {
        case 'a':
            type = check_keyword(1, 2, "nd", token_and);
            break;
        case 'c':
            type = check_keyword(1, 4, "lass", token_class);
            break;
        case 'e':
            type = check_keyword(1, 3, "lse", token_else);
            break;
        case 'f':
            if (position() > 1)
            {
                switch (scanner.start[1])
                {
                    case 'a':
                        type = check_keyword(2, 3, "lse", token_false);
                        break;
                    case 'o':
                        type = check_keyword(2, 1, "r", token_for);
                        break;
                    case 'u':
                        type = check_keyword(2, 1, "n", token_fun);
                        break;
                    default:
                        break;
                }
            }
            break;
        case 'i':
            type = check_keyword(1, 1, "f", token_if);
            break;
        case 'n':
            type = check_keyword(1, 2, "il", token_nil);
            break;
        case 'o':
            type = check_keyword(1, 1, "r", token_or);
            break;
        case 'p':
            type = check_keyword(1, 4, "rint", token_print);
            break;
        case 'r':
            type = check_keyword(1, 5, "eturn", token_return);
            break;
        case 't':
            if (position() > 1)
            {
                switch (scanner.start[1])
                {
                    case 'h':
                        type = check_keyword(2, 2, "is", token_this);
                        break;
                    case 'r':
                        type = check_keyword(2, 2, "ue", token_true);
                    default:
                        break;
                }
            }
            break;
        case 'v':
            type = check_keyword(1, 2, "ar", token_var);
            break;
        case 'w':
            type = check_keyword(1, 4, "hile", token_while);
        default:
            break;
    }
    return type;
}

static Token identifier()
{
    char c = peek();
    while (is_alpha(c) || is_digit(c))
    {
        advance();
        c = peek();
    }
    Token_type type = identifier_type();
    return make(type);
}

static Token number()
{
    // Consume all numbers up to decimal point or end.
    char c = peek();
    while (is_digit(c))
    {
        advance();
        c = peek();
    }

    // Consume decimal point and following numbers.
    if (c == '.' && is_digit(peek_next()))
    {
        advance();
        c = peek();
        while (is_digit(c))
        {
            advance();
            c = peek();
        }
    }

    return make(token_number);
}

static Token string()
{
    while (peek() != '"' && !at_end())
    {
        if (peek() == '\n')
        {
            scanner.line++;
        }
        advance();
    }

    Token token;
    if (at_end())
    {
        token = make_error("Unterminated string.");
    }
    else
    {
        // Consume the closing quote.
        advance();
        token = make(token_string);
    }
    return token;
}

static Token scan(char c)
{
    Token token;
    switch (c)
    {
        case '(':
            token = make(token_left_paren);
            break;
        case ')':
            token = make(token_right_paren);
            break;
        case '{':
            token = make(token_left_brace);
            break;
        case '}':
            token = make(token_right_brace);
            break;
        case ';':
            token = make(token_semicolon);
            break;
        case ',':
            token = make(token_comma);
            break;
        case '.':
            token = make(token_dot);
            break;
        case '-':
            token = make(token_minus);
            break;
        case '+':
            token = make(token_plus);
            break;
        case '/':
            token = make(token_slash);
            break;
        case '*':
            token = make(token_star);
            break;
        case '!':
            token = match('=') ? make(token_bang_equal) : make(token_bang);
            break;
        case '=':
            token = match('=') ? make(token_equal_equal) : make(token_equal);
            break;
        case '<':
            token = match('=') ? make(token_less_equal) : make(token_less);
            break;
        case '>':
            token = match('=') ? make(token_greater_equal) : make(token_greater);
            break;
        case '"':
            token = string();
            break;
        default:
            token = make_error("Unexpected character.");
            break;
    }
    return token;
}

void init_scanner(const char* source)
{
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

Token scan_token()
{
    skip_whitespace();
    scanner.start = scanner.current;
    Token token;
    if (at_end())
    {
        token = make(token_eof);
    }
    else
    {
        char c = advance();
        if (is_alpha(c))
        {
            token = identifier();
        }
        else if (is_digit(c))
        {
            token = number();
        }
        else
        {
            token = scan(c);
        }
    }
    return token;
}
