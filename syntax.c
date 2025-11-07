#include "syntax.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct { char name[64]; int value; int used; } Var; //dont ask me why, idk why
static Var vars[128]; //dont ask me this, the rest I know it vert damn well, but not this line. Too simple to explain 

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

//-------------------statement
AST* parse_statement(Parser *ps) {
    //AST *lhs = parse_expr(ps); 
    if (ps->current.type == TOK_ID && (ps->next.type == TOK_ASSIGN  || ps->next.type == TOK_COMPOUND_MINUS 
        || ps->next.type == TOK_COMPOUND_PLUS || ps->next.type == TOK_COMPOUND_MUL || ps->next.type == TOK_COMPOUND_DIV )) {
       
        char name[64]; 
        strncpy(name, ps->current.lexeme, sizeof(name)-1);
        
        eat(ps, TOK_ID);
        Token op = ps->current; 
        eat(ps, ps->current.type); 
        
        AST *rhs = parse_expr(ps); // or parse_equality()
        AST *node = calloc(1,sizeof(*node)); 
        node->type = AST_ASSIGN;
        strncpy(node->name, name, sizeof(node->name)-1);
        node->op = op; 
        //n->left= node;
        node->right = rhs;
        return node;
    }
    return parse_expr(ps);
}
//-------------------expression 
AST* parse_expr(Parser *ps) {
    AST *node = parse_term(ps);
    while (ps->current.type == TOK_PLUS || ps->current.type == TOK_MINUS
        ||ps->current.type == TOK_BITWISE_AND|| ps->current.type == TOK_BITWISE_OR||ps->current.type == TOK_LESS || ps->current.type == TOK_GREATER
        ||ps->current.type == TOK_OR||ps->current.type == TOK_AND||ps->current.type == TOK_LESS_EQ || ps->current.type == TOK_GREATER_EQ 
        ||ps->current.type == TOK_SHIFT_LEFT || ps->current.type == TOK_SHIFT_RIGHT|| ps->current.type == TOK_EQ || ps->current.type == TOK_NOT_EQ) {
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

static AST* make_id(const char* s) {
    AST* node = calloc(1, sizeof(AST));   // allocate memory for the AST node
    node->type = AST_ID;                  // mark this node as an identifier
    strncpy(node->name, s, sizeof(node->name) - 1); // store the variable name
    node->name[sizeof(node->name) - 1] = '\0';      // make sure it's null-terminated
    return node;
}
//-------------------factor
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
            AST*var = make_id(tok.lexeme);
            if (ps->current.type == TOK_INCREMENT || ps->current.type == TOK_DECREMENT) 
            { 
                Token post = ps->current;
                eat(ps, ps->current.type);
                AST *node = calloc(1, sizeof(AST));
                node->type = AST_UNARY;        // new node type
                node->op = post;
                node->left = var;             // store operand on the left
                return node;
            }
            return var; // return make_id(tok.lexeme)
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
    // prefix increment/decrement 
        if (tok.type == TOK_INCREMENT || tok.type == TOK_DECREMENT) {
            eat(ps, tok.type);
            AST *var = parse_factor(ps);   // must be an identifier
            AST *node = calloc(1, sizeof(AST));
            node->type = AST_UNARY;        // new node type
            node->op = tok;
            node->right = var;             // store operand on the right
            return node;
        }
        if (tok.type == TOK_MINUS || tok.type == TOK_NOT|| tok.type == TOK_BITWISE_NOT) { // unary op
            eat(ps, tok.type);
            AST *rhs = parse_factor(ps); //parse the right hs
            AST *node = (AST*)calloc(1, sizeof(AST));
            node->type = AST_UNARY;
            node->op = tok; // minus
            node->right = rhs;
            return node;
        }

    syntax_error("expected number, '(', or '-'", tok);
    return NULL; // unreachable
}
//-------------------term---------------------- 
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
// evaluate the binary operation, arithmetic 
 int eval_binary( TokenType op, int left, int right) {

    switch (op) {
        // binary arithmertic
        case TOK_PLUS:  return left + right;
        case TOK_MINUS: return left - right;
        case TOK_MUL:   return left * right;
        case TOK_DIV: if (right == 0) {
                fprintf(stderr, "Runtime error: division by zero\n"); // error handler for the x/0 case
                exit(EXIT_FAILURE); }
            return left / right; // integer division
        // relational maths
        case TOK_LESS:         return left <  right;
        case TOK_GREATER:      return left >  right;
        case TOK_LESS_EQ:      return left <= right; 
        case TOK_GREATER_EQ:   return left >= right; 
        // equality 
        case TOK_EQ:           return left == right; // notworking 
        case TOK_NOT_EQ:       return left != right;
        // bitwise
        case TOK_BITWISE_AND:  return left &  right;
        case TOK_BITWISE_OR:   return left |  right;
        case TOK_BITWISE_XOR:  return left ^  right;
        // logical (non-zero = true). If prefer short-circuit, handle at AST level.
        case TOK_AND:          return (left != 0) && (right != 0);
        case TOK_OR:           return (left != 0) || (right != 0);
        //shifting
        case TOK_SHIFT_LEFT:   return left << right; 
        case TOK_SHIFT_RIGHT:  return left >> right;
        default:
            fprintf(stderr, "Runtime error: unknown operator\n");
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
            return eval_binary(node->op.type, left, right);
        }
        case AST_ASSIGN: {                      // handle assignment here
            int val = eval_ast_assignment(node->right);
            int *slot = slot_for(node->name);   // lhs must be an identifier name
             switch (node->op.type) {
                case TOK_ASSIGN:         *slot  = val;       break;
                case TOK_COMPOUND_PLUS:  *slot += val;       break;
                case TOK_COMPOUND_MINUS: *slot -= val;       break;
                case TOK_COMPOUND_MUL:   *slot *= val;       break;
                case TOK_COMPOUND_DIV:  if (val == 0) { fprintf(stderr, "Division by zero\n"); exit(1); }
                                        *slot /= val;
                                        break;
                default: fprintf(stderr, "Unknown assignment op\n");
                exit(1);
                }
                return *slot;
            }
        case AST_UNARY: {
            
            if ((node->op.type == TOK_INCREMENT || node->op.type == TOK_DECREMENT)&& node-> right) {
                int *slot = slot_for(node->right->name);
                if (node->op.type == TOK_INCREMENT) {(*slot)++;}
                else {(*slot)--; }
                return *slot;
            }
             if ((node->op.type == TOK_INCREMENT || node->op.type == TOK_DECREMENT)&& node-> left) {
                int *slot = slot_for(node->left->name);
                if (node->op.type == TOK_INCREMENT) {(*slot)++;}
                else {(*slot)--; }
                return *slot;
            }
            if (node->op.type == TOK_NOT) {
                int val = eval_ast_assignment(node->right);
                return !(val);
            }
            if (node->op.type == TOK_MINUS) {
                int val = eval_ast_assignment(node->right); // value is on the right side
                return -(val);
            }
            if (node->op.type == TOK_BITWISE_NOT) {
                int val = eval_ast_assignment(node->right); // value is on the right side
                return ~val;
            }
            
        }
    }
    fprintf(stderr,"Runtime error: bad AST node\n"); exit(EXIT_FAILURE);
}
static char *indent_next(const char *indent, int last) {
    size_t a = strlen(indent), b = 4;
    char *s = malloc(a + b + 1);
    memcpy(s, indent, a);
    memcpy(s + a, last ? "    " : "|   ", b);
    s[a + b] = '\0';
    return s;
}
void print_tree_ascii(const AST* n, const char* indent, int last){
  printf("%s%s", indent, last ? "`-" : "|--- ");
  switch(n->type){
    case AST_NUM: printf("NUM(%d)\n", n->value); break;
    case AST_ID:  printf("ID(%s)\n",  n->name); break;
    case AST_ASSIGN:printf("ASSIGN(%s)\n", n->op.lexeme);  break;
    case AST_UNARY:  printf("UNARY(%s)\n", n->op.lexeme); break;
    case AST_BINOP: printf("BIN('%s')\n", n->op.lexeme); break;
    default:        printf("?\n"); break;
  }
   int child_count = 0;
    const AST *kids[2];
    if (n->type == AST_BINOP || n->type == AST_ASSIGN||n->type == AST_UNARY) {
        if (n->left)  kids[child_count++]  = n->left;
        if (n->right) kids[child_count++]  = n->right;
    }
     if (child_count == 0) return;

    char *next = indent_next(indent, last);
   
    for (int i = 0; i < child_count; ++i) {
         print_tree_ascii(kids[i], next, i == child_count - 1);
    }
    free(next);
 /* char next[256]; snprintf(next,sizeof(next), "%s%s", indent, last ? "    " : "|   ");
  if (n->type==AST_BINOP){
    print_tree_ascii(n->left, next, 0);
    print_tree_ascii(n->right, next, 1);
  }*/
}

void free_ast(AST *node) {
    if (!node) return;
    free_ast(node->left);
    free_ast(node->right);
    free(node);
}
