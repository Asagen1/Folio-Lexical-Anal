#ifndef LEXICAL_ANALYZER_H
#define LEXICAL_ANALYZER_H

#define MAX_LEX 128

typedef enum {
    S_START, S_IDENT, S_INT, S_DOT, S_FLOAT, S_CHAR,
    S_OP,        // +, -
    S_OP2,       // ==, 
    S_ASSIGN, S_LT, S_GT, S_BANG,
    S_DELIM, S_DEAD

} State;

typedef enum {
    T_IDENT, T_KW
} TokenType;

void lexer(const char *string);

#endif