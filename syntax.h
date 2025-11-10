#ifndef SYNTAX_H
#define SYNTAX_H

#include "lexer.h"

typedef enum { 
    AST_NUM, 
    AST_ID,   //define AST ID
    AST_ASSIGN, 
    AST_BINOP,  // BINARY OPERATION
    AST_UNARY, 
    AST_IF,// for if
    AST_RETURN,
    AST_WHILE,
    AST_FUNC,
} ASTType; 

typedef struct AST {
    ASTType type;
    int value;         // for AST_NUM
    Token op;          // for AST_BINOP
    char name[128]; // for AST_ID
    struct AST *left;  // for AST_BINOP
    struct AST *right; // for AST_BINOP
    struct AST *cond; //condition for ternary op, ehhhh tired, if u read to this point please do it for me, I dont want to do ternary. 
    struct AST *stmnts[128]; // Store an array for AST_FUNC
    int stmnt_cnt;
} AST;

typedef struct {
    Lexer *lexer;
    Token current;
    Token next;
} Parser;

void parser_init(Parser *ps, Lexer *lx);

// Entry point: parse a full expression
AST* parse_expr(Parser *ps);
AST* parse_statement(Parser *ps);

// Evaluate AST to an int (uses integer division for '/')
int eval_ast_assignment(const AST *node);
void print_tree_ascii(const AST* n, const char* indent, int last);
void print_tree(const AST* n);
void print_tree_better(const AST* n);  // Profile-based tree printing with proper spacing
// Free AST
void free_ast(AST *node);

#endif // SYNTAX_H
