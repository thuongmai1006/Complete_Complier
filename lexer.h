#ifndef LEXER_H
#define LEXER_H
#include <stddef.h>
//-----------------Token Generation/ Definition------------------
typedef enum {
    TOK_INT,     // integer literal
    TOK_ID,      // identifier for single character
    TOK_ASSIGN, // assignment =
    TOK_EQ , // equal ==
    TOK_PLUS,    // '+'
    TOK_COMPOUND_PLUS, //+=
    TOK_INCREMENT,    // '++'
    TOK_DECREMENT,    // '--'
    TOK_MINUS,   // '-'
    TOK_COMPOUND_MINUS, //-=
    TOK_MUL,   // '*'
    TOK_COMPOUND_MUL, //*=
    TOK_DIV,  // '/'
    TOK_COMPOUND_DIV, // /=
    TOK_LPAREN,  // '('
    TOK_RPAREN,  // ')'
    TOK_EOF,      // end of file
    TOK_GREATER, // >  NEWLY ADDED
    TOK_GREATER_EQ, // >=  NEWLY ADDED
    TOK_LESS,// <
    TOK_LESS_EQ,// <=
    TOK_SHIFT_LEFT, // <<
    TOK_SHIFT_RIGHT,// >>
    TOK_BITWISE_AND, //  bitwise binary &
    TOK_AND, // LOGICAL AND &&
    TOK_BITWISE_XOR, //^
    TOK_BITWISE_OR, // |
    TOK_OR, // LOGICAL OR ||
    TOK_NOT, // LOGICAL NOT !
    TOK_NOT_EQ, // NOT EQ !=
    TOK_BITWISE_NOT, //~ 1'S COMPLEMENT
    TOK_IF, // "if" // newly add here
    TOK_ELSE, // else
    TOK_BREAK, //break
    TOK_DO, // do
    TOK_WHILE, // "while"
    TOK_RETURN, // "return"
    TOK_INT_VAR, // "int" => redundant - Thuong ahhh to define the unknown variables??
    TOK_LCURLY, // "{"
    TOK_RCURLY, // "}"
    TOK_SEMI, // ";"
    TOK_COMMA, //,
    TOK_VOID, // "void"
    TOK_FLOAT,
    TOK_DBL,
    TOK_ELIF, // "else if"
    TOK_FOR, // "for"
    //TOK_QUESTION, //?
    //TOK_COLON //  :
} TokenType;

typedef struct {
    TokenType type;
    int value;      // only valid when type == TOK_INTEGER
    char lexeme[128];
    size_t pos;     // index in source
    size_t id_no;
} Token;

typedef struct {
    const char *input;
    size_t pos;
    char current;
    size_t id_cnt;
} Lexer;

void lexer_init(Lexer *lex, char *input);
Token lexer_next_token(Lexer *lex);

#endif // LEXER_H
