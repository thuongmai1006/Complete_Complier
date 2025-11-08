#ifndef SYNTAX_H
#define SYNTAX_H

#include "lexer.h"

typedef enum { 
    AST_NUM, 
    AST_KEY, 
    AST_ID,   //define AST ID
    AST_ASSIGN, 
    AST_BINOP,  // BINARY OPERATION
    AST_UNARY, 
    AST_FUNC,
    AST_PROG,
    AST_BLOCK,
    AST_RET,
} ASTType; 

typedef struct AST {
    ASTType type;
    int value;         // for AST_NUM
    Token op;          // for AST_BINOP
    char name[128]; // for AST_ID
    struct AST *left;  // for AST_BINOP
    struct AST *right; // for AST_BINOP
    struct AST* expr;
    struct AST* body;
    struct AST* children[16];
    int child_cnt;
} AST;

typedef struct {
    Lexer *lexer;
    Token current;
    Token next;
} Parser;

void parser_init(Parser *ps, Lexer *lx);

// Entry point: parse a full expression
AST* parse();
/*AST* parse_expr(Parser *ps, AST* root);
AST* parse_statement(Parser *ps);
*/
// Evaluate AST to an int (uses integer division for '/')
int eval_ast_assignment(const AST *node);
void print_tree_ascii(const AST* n, const char* indent, int last);
void print_tree_better(const AST* n);  // Profile-based tree printing with proper spacing
// Free AST
void free_ast(AST *node);

#endif // SYNTAX_H
