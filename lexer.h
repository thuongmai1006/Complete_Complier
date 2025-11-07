#ifndef LEXER_H
#define LEXER_H
#include <stddef.h>
//-----------------Token Generation/ Definition------------------
typedef enum {
    TOK_INT,     // integer literal
    TOK_ID,      // identifier
    TOK_ASSIGN, // assignment =
    TOK_EQ , // equal ==
    TOK_PLUS,    // '+'
    TOK_MINUS,   // '-'
    TOK_MUL,   // '*'
    TOK_DIV,  // '/'
    TOK_LPAREN,  // '('
    TOK_RPAREN,  // ')'
    TOK_EOF      // end of file
} TokenType;

typedef struct {
    TokenType type;
    int value;      // only valid when type == TOK_INTEGER
    char lexeme[64];
    size_t pos;     // index in source
} Token;

typedef struct {
    const char *input;
    size_t pos;
    char current;
} Lexer;

void lexer_init(Lexer *lex, const char *input);
Token lexer_next_token(Lexer *lex);

#endif // LEXER_H