#include "syntax.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct { char name[64]; int value; int used; } Var;
static Var vars[128];
static void syntax_error(const char *msg, Token got) {
    fprintf(stderr, "Syntax error at %zu: %s (got token %d)\n",
            got.pos, msg, got.type);
    exit(EXIT_FAILURE);
}

static void eat(Parser *ps, TokenType expect) {
    if (ps->current.type == expect) {
        ps->current =  ps->next;
        ps->next = lexer_next_token(ps->lexer);
    } else {
        syntax_error("unexpected token", ps->current);
    }
}


// Forward decls
static AST* parse_factor(Parser *ps);
static AST* parse_term(Parser *ps);

// Grammar:
// expr   : term ((PLUS|MINUS) term)*
// term   : factor ((MUL|DIV) factor)*
// factor : INTEGER | LPAREN expr RPAREN | MINUS factor
AST* parse_expr(Parser *ps) {
    AST *node = parse_term(ps);
    while (ps->current.type == TOK_PLUS || ps->current.type == TOK_MINUS) {
        Token op = ps->current;
        eat(ps, op.type);
        AST *rhs = parse_term(ps);

        AST *bin = (AST*)calloc(1, sizeof(AST));
        bin->type = AST_BINOP;
        bin->op = op;
        bin->left = node;
        bin->right = rhs;
        node = bin;
    }
    return node;
}
AST* parse_statement(Parser *ps) {
    if (ps->current.type == TOK_ID && ps->next.type == TOK_ASSIGN) {
        char name[64]; strncpy(name, ps->current.lexeme, sizeof(name)-1);
        eat(ps, TOK_ID);
        eat(ps, TOK_ASSIGN);
        AST *rhs = parse_expr(ps); // or parse_equality()
        AST *n = calloc(1,sizeof(*n)); n->type = AST_ASSIGN;
        strncpy(n->name, name, sizeof(n->name)-1);
        n->right = rhs;
        return n;
    }
    return parse_expr(ps);
}

static AST* make_id(const char* s) {
    AST* n = calloc(1, sizeof(AST));   // allocate memory for the AST node
    n->type = AST_ID;                  // mark this node as an identifier
    strncpy(n->name, s, sizeof(n->name) - 1); // store the variable name
    n->name[sizeof(n->name) - 1] = '\0';      // make sure it's null-terminated
    return n;
}

static AST* parse_factor(Parser *ps) {
    Token tok = ps->current;

    if (tok.type == TOK_INT) {
        eat(ps, TOK_INT);
        AST *num = (AST*)calloc(1, sizeof(AST));
        num->type = AST_NUM;
        num->value = tok.value;
        return num;
    }
     if (tok.type ==TOK_ID)
    {
         eat(ps, TOK_ID);
         return make_id(tok.lexeme);
    }
    if (tok.type == TOK_LPAREN) {
        eat(ps, TOK_LPAREN);
        AST *inner = parse_expr(ps);
        if (ps->current.type != TOK_RPAREN) {
            syntax_error("expected ')'", ps->current);
        }
        eat(ps, TOK_RPAREN);
        return inner;
    }
    if (tok.type == TOK_MINUS) { // unary minus
        eat(ps, TOK_MINUS);
        AST *zero = (AST*)calloc(1, sizeof(AST));
        zero->type = AST_NUM;
        zero->value = 0;

        AST *rhs = parse_factor(ps);

        AST *bin = (AST*)calloc(1, sizeof(AST));
        bin->type = AST_BINOP;
        bin->op = tok; // minus
        bin->left = zero;
        bin->right = rhs;
        return bin;
    }
   

    syntax_error("expected number, '(', or '-'", tok);
    return NULL; // unreachable
}

static AST* parse_term(Parser *ps) {
    AST *node = parse_factor(ps);
    while (ps->current.type == TOK_MUL || ps->current.type == TOK_DIV) {
        Token op = ps->current;
        eat(ps, op.type);
        AST *rhs = parse_factor(ps);

        AST *bin = (AST*)calloc(1, sizeof(AST));
        bin->type = AST_BINOP;
        bin->op = op;
        bin->left = node;
        bin->right = rhs;
        node = bin;
    }
    return node;
}

void parser_init(Parser *ps, Lexer *lx) {
    ps->lexer = lx;
    ps->current = lexer_next_token(lx);
    ps->next    = lexer_next_token(lx);
}

 int eval_ast( TokenType op, int left, int right) {
   

    switch (op) {
        case TOK_PLUS:  return left + right;
        case TOK_MINUS: return left - right;
        case TOK_MUL:   return left * right;
        case TOK_DIV:
            if (right == 0) {
                fprintf(stderr, "Runtime error: division by zero\n"); // error handler for the x/0 case
                exit(EXIT_FAILURE);
            }
            return left / right; // integer division
        default:
            fprintf(stderr, "Runtime error: bad operator\n");
            exit(EXIT_FAILURE);
    }
}
static int* slot_for(const char* name) {
    for (int i=0;i<128;i++) 
    {if (vars[i].used && strcmp(vars[i].name,name)==0) return &vars[i].value;}
    for (int i=0;i<128;i++) 
    {if (!vars[i].used) { vars[i].used=1; strncpy(vars[i].name,name,63); vars[i].name[63]=0; return &vars[i].value; } }
    fprintf(stderr,"Symbol table full\n"); exit(EXIT_FAILURE);
}
static int get_var(const char* name) {
    for (int i=0;i<128;i++) {
    if (vars[i].used && strcmp(vars[i].name,name)==0) 
    return vars[i].value;
    }
    fprintf(stderr,"Undefined variable '%s'\n", name);
    exit(EXIT_FAILURE);
}
int eval_ast_assignment(const AST *node) {
    switch (node->type) {
        case AST_NUM:  return node->value;
        case AST_ID:   return get_var(node->name);

        case AST_BINOP: {
            int left = eval_ast_assignment(node->left);
            int right = eval_ast_assignment(node->right);
            return eval_ast(node->op.type, left, right);
        }

        case AST_ASSIGN: {                      // <-- handle assignment here
            int val = eval_ast_assignment(node->right);
            int *slot = slot_for(node->name);   // lhs must be an identifier name
            *slot = val;
            return val;                         // many REPLs return assigned value
        }
    }
    fprintf(stderr,"Runtime error: bad AST node\n"); exit(EXIT_FAILURE);
}
void free_ast(AST *node) {
    if (!node) return;
    free_ast(node->left);
    free_ast(node->right);
    free(node);
}
