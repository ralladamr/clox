#ifndef clox_scanner
#define clox_scanner

typedef enum
{
    // One-character tokens.
    token_left_paren,
    token_right_paren,
    token_left_brace,
    token_right_brace,
    token_comma,
    token_dot,
    token_minus,
    token_plus,
    token_semicolon,
    token_slash,
    token_star,

    // One- or two-character tokens.
    token_bang,
    token_bang_equal,
    token_equal,
    token_equal_equal,
    token_greater,
    token_greater_equal,
    token_less,
    token_less_equal,

    // Literals.
    token_identifier,
    token_string,
    token_number,

    // Keywords.
    token_and,
    token_class,
    token_else,
    token_false,
    token_for,
    token_fun,
    token_if,
    token_nil,
    token_or,
    token_print,
    token_return,
    token_super,
    token_this,
    token_true,
    token_var,
    token_while,

    // Other tokens.
    token_error,
    token_eof
} Token_type;

typedef struct
{
    Token_type type;
    const char* start;
    int length;
    int line;
} Token;

void init_scanner(const char* source);
Token scan_token();

#endif

