#ifndef SYNTAX_H
#define SYNTAX_H

#include "lexer.h"

typedef enum { 
    AST_NUM, 
    AST_ID,   //define AST op
    AST_ASSIGN,
    AST_BINOP } 
    ASTType;

typedef struct AST {
    ASTType type;
    int value;         // for AST_NUM
    Token op;          // for AST_BINOP
    char name[64]; // for AST_ID
    struct AST *left;  // for AST_BINOP
    struct AST *right; // for AST_BINOP
} AST;

typedef struct {
    Lexer *lexer;
    Token current;
} Parser;

void parser_init(Parser *ps, Lexer *lx);

// Entry point: parse a full expression
AST* parse_expr(Parser *ps);

// Evaluate AST to an int (uses integer division for '/')
int eval_ast_assignment(const AST *node);

// Free AST
void free_ast(AST *node);

#endif // SYNTAX_H
