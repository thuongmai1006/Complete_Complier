#ifndef SYNTAX_H
#define SYNTAX_H
#define FN_STMTS_CAP 128
#include "lexer.h"
#include <string.h>
typedef enum
{
    AST_NUM,
    AST_ID, // define AST ID
    AST_ASSIGN,
    AST_BINOP, // BINARY OPERATION
    AST_UNARY,
    AST_IF, // for if
    AST_ELIF,
    AST_ELSE,
    AST_RETURN,
    AST_WHILE,
    AST_FUNC,
    AST_BLOCK,
    AST_FOR,
    AST_DECLARATION
} ASTType;

typedef struct AST
{
    ASTType type;
    double value;            // for AST_NUM
    Token op;                // for AST_BINOP
    char name[128];          // for AST_ID
    struct AST *left;        // for AST_BINOP
    struct AST *right;       // for AST_BINOP
    struct AST *cond;        // condition for ternary op, ehhhh tired, if u read to this point please do it for me, I dont want to do ternary.
    struct AST *stmnts[128]; // Store an array for AST_FUNC
    struct AST *params[16];
    struct AST *init; // init variable declaration for loop for

    int used;
    int stmnt_cnt; // what is this doing?
    int param_cnt; // what is this doing?
} AST;

typedef struct
{
    Lexer *lexer;
    Token current;
    Token next;
} Parser;

typedef struct
{
    char name[64];
    // int value;
    double used;
    Token op;
    union
    {
        int i;
        float f;
    } num_val;
} Var; // dont ask me why, idk why

static Var vars[128];
void parser_init(Parser *ps, Lexer *lx);

// Entry point: parse a full expression
AST *parse_expr(Parser *ps);
AST *parse_fn(Parser *ps);
AST *parse_statement(Parser *ps);

double eval_ast_decl(AST *n);
void advance(Parser *ps);
// Evaluate AST to an int (uses integer division for '/')
double eval_ast_assignment(AST *node);
static double eval_function_call(AST *fn, AST **args, int arg_count);
void print_tree_ascii(const AST *n, const char *indent, int last);
void print_tree(const AST *n);
void print_tree_better(const AST *n); // Profile-based tree printing with proper spacing
// Free AST
void free_ast(AST *node);
#endif // SYNTAX_H
