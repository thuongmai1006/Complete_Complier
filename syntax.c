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

static void eat(Parser *ps, TokenType expect) { // eat is expecting the token 
    if (ps->current.type == expect) {  
        ps->current =  ps->next;
        ps->next = lexer_next_token(ps->lexer);
    } else {
        syntax_error("unexpected token", ps->current);
    }
}

static int is_returnType(Parser* ps){
    if (ps->current.type == TOK_INT_VAR || ps->current.type == TOK_VOID
            || ps->current.type == TOK_FLOAT || ps->current.type == TOK_DBL){
        return 1;
    } else return 0;
}
static int is_assignment(Parser* ps){
    if (ps->current.type == TOK_ID && (ps->next.type == TOK_ASSIGN  || ps->next.type == TOK_COMPOUND_MINUS 
        || ps->next.type == TOK_COMPOUND_PLUS || ps->next.type == TOK_COMPOUND_MUL || ps->next.type == TOK_COMPOUND_DIV)){
        return 1;
    } else return 0;
}

// Forward decls
static AST* parse_factor(Parser *ps);
static AST* parse_term(Parser *ps);
static AST* parse_assignment(Parser*);
static AST* parse_block(Parser*);
static AST* parse_if(Parser*);
static AST* parse_while(Parser*);
static AST* parse_return(Parser*);
static AST* parse_id(Parser*);
static AST* make_id(const char*);
// Grammar:
// expr   : term ((PLUS|MINUS) term)*
// term   : factor ((MUL|DIV) factor)*
// factor : INTEGER | LPAREN expr RPAREN | MINUS factor

//-------------------statement
static AST* parse_id(Parser* ps){
    eat(ps, TOK_ID);
    AST *var = make_id(ps->current.lexeme);
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

static void eat_returnType(Parser* ps){
    switch(ps->current.type){
        case TOK_INT_VAR:
            eat(ps, TOK_INT_VAR);
            break;
        case TOK_DBL:
            eat(ps, TOK_DBL);
            break;
        case TOK_FLOAT:
            eat(ps, TOK_FLOAT);
            break;
        case TOK_VOID:
            eat(ps, TOK_VOID);
            break;
        default:
            eat(ps, TOK_EOF); // throw an error
    }
}
/* Examples:
 * int main() {}
 * int sum(int a, int b){return a + b;}
 */
AST* parse_fn(Parser *ps){
    eat_returnType(ps);
    eat(ps, TOK_ID);
    eat(ps, TOK_LPAREN);
    while (ps->current.type != TOK_RPAREN){
        eat_returnType(ps);
        eat(ps, TOK_ID);
        if (ps->current.type == TOK_COMMA){
            eat(ps, TOK_COMMA);
        }
    }
    eat(ps, TOK_RPAREN);
    return parse_block(ps);
}

AST* parse_statement(Parser *ps) {
    if (is_assignment(ps)) {
        return parse_assignment(ps);   
    }

    if (ps->current.type == TOK_IF){
        return parse_if(ps);
    }

    if (ps->current.type == TOK_WHILE){
        return parse_while(ps);
    }

    if (ps->current.type == TOK_RETURN){
        return parse_return(ps);
    }
    
    if (ps->current.type == TOK_LCURLY){
        return parse_block(ps);
    }

    return parse_expr(ps);
}

static AST* parse_block(Parser* ps){
    AST* inner_block = NULL;
    eat(ps, TOK_LCURLY);
    while (ps->current.type != TOK_RCURLY){
        inner_block = parse_statement(ps);
    }
    eat(ps, TOK_RCURLY);
    return inner_block;
}
static AST* parse_assignment(Parser* ps){
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
    node->right = rhs;
    return node;

}
static AST* parse_if (Parser *ps){
    eat (ps, TOK_IF);
    eat (ps, TOK_LPAREN); // not sure here -Thuong 
    AST *if_cond = parse_expr(ps); // parse the if condition inside the ()
    if (ps->current.type != TOK_RPAREN) { syntax_error("expected ')'", ps->current); }
    eat(ps, TOK_RPAREN);
    return if_cond;
}

static AST* parse_while(Parser* ps){
    eat(ps, TOK_WHILE);
    eat(ps, TOK_LPAREN);
    AST *cond = parse_expr(ps);
    eat(ps, TOK_RPAREN);
    return cond;
}

static AST* parse_return(Parser* ps){
    eat(ps, TOK_RETURN);
    return parse_expr(ps);
}

int is_binOp(int curr){
    if (curr == TOK_PLUS || curr == TOK_MINUS
        ||curr == TOK_BITWISE_AND|| curr == TOK_BITWISE_OR||curr == TOK_LESS || curr == TOK_GREATER
        ||curr == TOK_OR||curr == TOK_AND||curr == TOK_LESS_EQ || curr == TOK_GREATER_EQ 
        ||curr == TOK_SHIFT_LEFT || curr == TOK_SHIFT_RIGHT|| curr == TOK_EQ || curr == TOK_NOT_EQ){
        return 1;
    }
    else return 0;
}
//-------------------expression 
AST* parse_expr(Parser *ps) {
    AST *node = parse_term(ps);
    while (is_binOp(ps->current.type)) {
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
        // if integer literal, create an AST node with type AST_NUM and value of the int
        if (tok.type == TOK_INT) {
            eat(ps, TOK_INT);
            AST *num = (AST*)calloc(1, sizeof(AST));
            num->type = AST_NUM;
            num->value = tok.value;
            return num;
        }
        // if the token is an identifier, create an AST node with type AST_ID 
        if (tok.type == TOK_ID)
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
        // parse the inner expression of a left, right parenthesis pair
        if (tok.type == TOK_LPAREN) {
            eat(ps, TOK_LPAREN);
            AST *inner_primary = parse_expr(ps);
            if (ps->current.type != TOK_RPAREN) {
                syntax_error("expected ')'", ps->current);
            }
            eat(ps, TOK_RPAREN);
            return inner_primary;
        }
        // Ivan applied {}
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
        if (tok.type == TOK_SEMI){
            return NULL;
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
  printf("%s%s", indent, last ? " \\_" : "|--- ");
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
// prototypes
void print_tree(const AST*);
void print_tree_better(const AST*);

// Better tree printing with profile-based layout
#define MAX_HEIGHT 1000
static int lprofile[MAX_HEIGHT];
static int rprofile[MAX_HEIGHT];
#define INFINITY (1<<20)
static int print_next;
static int gap = 3;

typedef struct ASTWrapper {
    struct ASTWrapper *left, *right;
    const AST *node;
    int edge_length;
    int height;
    int label_width;
    int parent_dir; // -1=left, 0=root, 1=right
    char label[64];
} ASTWrapper;

void free_ast(AST *node) {
    if (!node) return;
    free_ast(node->left);
    free_ast(node->right);
    free(node);
}

// Better tree printing implementation
static ASTWrapper *build_wrapper_tree(const AST *node) {
    if (!node) return NULL;

    ASTWrapper *wrapper = malloc(sizeof(ASTWrapper));
    wrapper->node = node;
    wrapper->left = build_wrapper_tree(node->left);
    wrapper->right = build_wrapper_tree(node->right);

    if (wrapper->left) wrapper->left->parent_dir = -1;
    if (wrapper->right) wrapper->right->parent_dir = 1;

    // Format label based on node type
    switch(node->type) {
        case AST_NUM: sprintf(wrapper->label, "NUM(%d)", node->value); break;
        case AST_ID:  sprintf(wrapper->label, "ID(%s)", node->name); break;
        case AST_ASSIGN: sprintf(wrapper->label, "ASSIGN(%s)", node->op.lexeme); break;
        case AST_UNARY:  sprintf(wrapper->label, "UNARY(%s)", node->op.lexeme); break;
        case AST_BINOP: sprintf(wrapper->label, "BIN('%s')", node->op.lexeme); break;
        default: sprintf(wrapper->label, "?"); break;
    }
    wrapper->label_width = strlen(wrapper->label);

    return wrapper;
}

static void free_wrapper_tree(ASTWrapper *node) {
    if (!node) return;
    free_wrapper_tree(node->left);
    free_wrapper_tree(node->right);
    free(node);
}

static int MIN(int x, int y) { return x < y ? x : y; }
static int MAX(int x, int y) { return x > y ? x : y; }

static void compute_lprofile(ASTWrapper *node, int x, int y) {
    if (!node) return;
    int isleft = (node->parent_dir == -1);
    lprofile[y] = MIN(lprofile[y], x - ((node->label_width - isleft) / 2));
    if (node->left) {
        for (int i = 1; i <= node->edge_length && y + i < MAX_HEIGHT; i++) {
            lprofile[y + i] = MIN(lprofile[y + i], x - i);
        }
    }
    compute_lprofile(node->left, x - node->edge_length - 1, y + node->edge_length + 1);
    compute_lprofile(node->right, x + node->edge_length + 1, y + node->edge_length + 1);
}

static void compute_rprofile(ASTWrapper *node, int x, int y) {
    if (!node) return;
    int notleft = (node->parent_dir != -1);
    rprofile[y] = MAX(rprofile[y], x + ((node->label_width - notleft) / 2));
    if (node->right) {
        for (int i = 1; i <= node->edge_length && y + i < MAX_HEIGHT; i++) {
            rprofile[y + i] = MAX(rprofile[y + i], x + i);
        }
    }
    compute_rprofile(node->left, x - node->edge_length - 1, y + node->edge_length + 1);
    compute_rprofile(node->right, x + node->edge_length + 1, y + node->edge_length + 1);
}

static void compute_edge_lengths(ASTWrapper *node) {
    if (!node) return;
    compute_edge_lengths(node->left);
    compute_edge_lengths(node->right);

    if (!node->right && !node->left) {
        node->edge_length = 0;
    } else {
        int hmin = 0;
        if (node->left) {
            for (int i = 0; i < node->left->height && i < MAX_HEIGHT; i++) {
                rprofile[i] = -INFINITY;
            }
            compute_rprofile(node->left, 0, 0);
            hmin = node->left->height;
        }
        if (node->right) {
            for (int i = 0; i < node->right->height && i < MAX_HEIGHT; i++) {
                lprofile[i] = INFINITY;
            }
            compute_lprofile(node->right, 0, 0);
            hmin = node->right ? MIN(node->right->height, hmin) : hmin;
        }

        int delta = 4;
        for (int i = 0; i < hmin; i++) {
            delta = MAX(delta, gap + 1 + rprofile[i] - lprofile[i]);
        }

        if (((node->left && node->left->height == 1) ||
             (node->right && node->right->height == 1)) && delta > 4) {
            delta--;
        }

        node->edge_length = ((delta + 1) / 2) - 1;
    }

    int h = 1;
    if (node->left) h = MAX(node->left->height + node->edge_length + 1, h);
    if (node->right) h = MAX(node->right->height + node->edge_length + 1, h);
    node->height = h;
}

static void print_level(ASTWrapper *node, int x, int level) {
    if (!node) return;
    int isleft = (node->parent_dir == -1);

    if (level == 0) {
        for (int i = 0; i < (x - print_next - ((node->label_width - isleft) / 2)); i++) {
            printf(" ");
        }
        print_next += (x - print_next - ((node->label_width - isleft) / 2));
        printf("%s", node->label);
        print_next += node->label_width;
    } else if (node->edge_length >= level) {
        if (node->left) {
            for (int i = 0; i < (x - print_next - level); i++) {
                printf(" ");
            }
            print_next += (x - print_next - level);
            printf("/");
            print_next++;
        }
        if (node->right) {
            for (int i = 0; i < (x - print_next + level); i++) {
                printf(" ");
            }
            print_next += (x - print_next + level);
            printf("\\");
            print_next++;
        }
    } else {
        print_level(node->left, x - node->edge_length - 1, level - node->edge_length - 1);
        print_level(node->right, x + node->edge_length + 1, level - node->edge_length - 1);
    }
}

void print_tree_better(const AST *root) {
    if (!root) return;

    ASTWrapper *wrapper = build_wrapper_tree(root);
    wrapper->parent_dir = 0;
    compute_edge_lengths(wrapper);

    for (int i = 0; i < wrapper->height && i < MAX_HEIGHT; i++) {
        lprofile[i] = INFINITY;
    }
    compute_lprofile(wrapper, 0, 0);

    int xmin = 0;
    for (int i = 0; i < wrapper->height && i < MAX_HEIGHT; i++) {
        xmin = MIN(xmin, lprofile[i]);
    }

    for (int i = 0; i < wrapper->height; i++) {
        print_next = 0;
        print_level(wrapper, -xmin, i);
        printf("\n");
    }

    free_wrapper_tree(wrapper);
}
