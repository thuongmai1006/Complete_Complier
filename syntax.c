#include "syntax.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct { char name[64]; int value; int used; } Var; //dont ask me why, idk why
static Var vars[128]; //dont ask me this, the rest I know it vert damn well, but not this line. Too simple to explain 
// error handling
static void syntax_error(const char *msg, Token got) {
    fprintf(stderr, "Syntax error at %zu: %s (got token %d)\n",
            got.pos, msg, got.type);
    exit(EXIT_FAILURE);
}
// eat is expecting the token 
static void eat(Parser *ps, TokenType expect) { 
    if (ps->current.type == expect) {  
        ps->current =  ps->next;
        ps->next = lexer_next_token(ps->lexer);
    } else {
        syntax_error("unexpected token", ps->current);
    }

}
// return expecting token
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
            eat(ps, TOK_EOF); // throw an error ???
    }
}
// retunr the token return type 
static int is_returnType(Parser* ps){
    if (ps->current.type == TOK_INT_VAR || ps->current.type == TOK_VOID
            || ps->current.type == TOK_FLOAT || ps->current.type == TOK_DBL){
        return 1;
    } else return 0;
}
// return the assignment type 
static int is_assignment(Parser* ps){
    if (ps->current.type == TOK_ASSIGN  || ps->current.type == TOK_COMPOUND_MINUS 
        || ps->current.type == TOK_COMPOUND_PLUS || ps->current.type == TOK_COMPOUND_MUL || ps->current.type == TOK_COMPOUND_DIV){
        return 1;
    } else return 0;
}
// return binary 
int is_binOp(int curr){
    if (curr == TOK_PLUS || curr == TOK_MINUS
        ||curr == TOK_BITWISE_AND|| curr == TOK_BITWISE_OR||curr == TOK_LESS || curr == TOK_GREATER
        ||curr == TOK_OR||curr == TOK_AND||curr == TOK_LESS_EQ || curr == TOK_GREATER_EQ 
        ||curr == TOK_SHIFT_LEFT || curr == TOK_SHIFT_RIGHT|| curr == TOK_EQ || curr == TOK_NOT_EQ
        || curr == TOK_MUL || curr == TOK_DIV){
        return 1;
    }
    else return 0;
}
// return single unary operations
int is_unOp(int curr){
    if (curr == TOK_BITWISE_NOT || curr == TOK_NOT
        || curr == TOK_MINUS || curr == TOK_INCREMENT || curr == TOK_DECREMENT){
            return 1;
        } else return 0;
}
// make ID for the identifier
static AST* make_id(const char* s) {
    AST* node = calloc(1, sizeof(AST));   // allocate memory for the AST node
    node->type = AST_ID;                  // mark this node as an identifier
    strncpy(node->name, s, sizeof(node->name) - 1); // store the variable name
    node->name[sizeof(node->name) - 1] = '\0';      // make sure it's null-terminated
    return node;
}
// AST for function 
AST* make_fn(const char* name){
    AST* fn = make_id(name);
    fn->type = AST_FUNC; 
    fn->stmnt_cnt = 0;
    return fn;
}



static AST* make_return(char* name, AST* expr){
    AST* ret = make_id(name);
    ret->type = AST_RETURN;
    ret->right = expr;
    return ret;
}

// Grammar:
// expr   : term ((PLUS|MINUS) term)*
// term   : factor ((MUL|DIV) factor)*
// factor : INTEGER | LPAREN expr RPAREN | MINUS factor

/* Examples:
 * int main() {}
 * int sum(int a, int b){return a + b;}
 */
// Forward decls
static AST* parse_factor(Parser *ps);
static AST* parse_term(Parser *ps);
static AST* parse_assignment(Parser*);
static AST* parse_block(Parser*);
//static AST* parse_if(Parser*);
//static AST* parse_while(Parser*);
static AST* parse_return(Parser*);
static AST* parse_id(Parser*);
static AST* parse_definition(Parser*);

// parse function first 
AST* parse_fn(Parser *ps){
    AST* fn;
    eat_returnType(ps);
    if (ps->current.type == TOK_ID){
        fn = make_fn(ps->current.lexeme);
    }
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
    // start body {}
    eat(ps, TOK_LCURLY);
    // check if while loop is made safe by EOF marker
    while (ps->current.type != TOK_RCURLY){
        fn->stmnts[fn->stmnt_cnt++] = parse_statement(ps);
    }
    eat(ps, TOK_RCURLY);
    return fn;
}
void eat_SEMI(Parser* ps){
    if (ps->current.type == TOK_SEMI){
        eat(ps, TOK_SEMI);
    } else {
        syntax_error("Expected ';' after expression",ps->current);
    }
}
// parse statement 
AST* parse_statement(Parser *ps) {
    // Variable definition
    if (is_returnType(ps)) {
        AST* defn = parse_definition(ps);
        eat_SEMI(ps);
        return defn;   
    }
    // Conditional Statement
    if (ps->current.type == TOK_IF){
        return parse_term (ps);
    }
    // Loop Statement
    if (ps->current.type == TOK_WHILE){
        return parse_term(ps); // you parse_while here when u already move the parse_while logic into the parse term
    }
    // Return Statement
    if (ps->current.type == TOK_RETURN){
        AST* ret = parse_return(ps);
        eat_SEMI(ps);
        return ret;
    }
    // Compound Statement 
    if (ps->current.type == TOK_LCURLY){
        return parse_block(ps);   // handle the {} separately by using parse_block rather than do it inside while
    }
    // Empty Statement
    if (ps->current.type == TOK_SEMI){
        eat_SEMI(ps);
        return NULL;
    }
    // Expression Statement
    AST* expr = parse_expr(ps);
    eat_SEMI(ps); // semi expecting after all operations. 
    return expr;
}
// parse definition here like int
static AST* parse_definition(Parser* ps){
    eat_returnType(ps);
    char name[64]; 
    strncpy(name, ps->current.lexeme, sizeof(name)-1);
    eat(ps, TOK_ID);
    Token eq = ps->current;
    eat(ps, TOK_ASSIGN);
    AST* right = parse_expr(ps);
    AST* node = calloc(1, sizeof(*node));
    node->type = AST_ASSIGN;
    node->op = eq; 
    node->right = right;
    node->left = make_id(name);
    return node;
}



static AST* parse_return(Parser* ps){
    char name[64]; 
    strncpy(name, ps->current.lexeme, sizeof(name)-1);
    eat(ps, TOK_RETURN);
    AST* expr = parse_expr(ps);
    eat(ps, TOK_SEMI);
    return make_return(name, expr);
}

// ivan move {} to become parse block 
static AST* parse_block(Parser* ps){
    AST* inner_block = NULL;
    eat(ps, TOK_LCURLY);
    while (ps->current.type != TOK_RCURLY){
        inner_block = parse_statement(ps);
    }
    eat(ps, TOK_RCURLY);
    return inner_block;
}

//-------------------expression 
AST* parse_expr(Parser *ps) {
    AST *node = parse_term(ps); // eats the term token
    if (ps->current.type == TOK_INCREMENT || ps->current.type == TOK_DECREMENT){
        Token post = ps->current;
        eat(ps, ps->current.type);
        AST *postfix = calloc(1, sizeof(AST));
        postfix->type = AST_UNARY;        // new node type
        postfix->op = post;
        postfix->left = node;             // store operand on the left
        node = postfix;
    }
    while (is_binOp(ps->current.type)) {
        Token op = ps->current;
        eat(ps, op.type);
        AST *expr = parse_expr(ps);
        AST *binOp = (AST*)calloc(1, sizeof(AST));
        binOp->type = AST_BINOP;
        binOp->op = op;
        binOp->left = node;
        binOp->right = expr;
        node = binOp;
    }
    if (is_assignment(ps)){
        Token op = ps->current;
        eat(ps, ps->current.type);
        AST* expr = parse_expr(ps);
        AST* assign = (AST*)calloc(1, sizeof(AST));
        assign->op = op;
        assign->type = AST_ASSIGN;
        assign->left = node;
        assign->right = expr;
        node = assign;
    }
    return node;
}

//-------------------term---------------------- 
static AST* parse_term(Parser* ps){
    Token tok = ps->current;
    if(tok.type == TOK_IF){
        AST* if_AST = (AST*)calloc(1, sizeof(AST));
        if_AST->type = AST_IF;
        eat(ps, TOK_IF);
        eat(ps, TOK_LPAREN);
        if_AST->cond = parse_expr(ps);
        eat(ps, TOK_RPAREN);
        eat(ps, TOK_LCURLY);
        if_AST->left = parse_statement(ps); //then 
        //if_AST->right = NULL;
        eat(ps, TOK_RCURLY);
        if (ps->current.type == TOK_ELSE ){ //
           if_AST->right = parse_term (ps); 
        }
       
        return if_AST;
    }
    /*if(tok.type == TOK_ELIF){
        AST* elif_AST = (AST*)calloc(1, sizeof(AST));
        elif_AST->type = AST_ELIF;
        eat(ps, TOK_ELIF);
        eat(ps, TOK_LPAREN);
        elif_AST->cond = parse_expr(ps);
        eat(ps, TOK_RPAREN);
        eat(ps, TOK_LCURLY);
        elif_AST->left = parse_statement(ps);
        eat(ps, TOK_RCURLY);
        if (ps->current.type == TOK_ELSE || ps->current.type == TOK_ELIF){
           elif_AST->right = parse_term(ps); 
        }
        return elif_AST;
    }
         */
    if (tok.type == TOK_ELSE){
        AST* else_AST = (AST*)calloc(1, sizeof(AST));
        else_AST->type = AST_ELSE;
        eat(ps, TOK_ELSE);
        eat(ps, TOK_LCURLY);
        else_AST->left = parse_statement(ps);
        eat(ps, TOK_RCURLY);
        return else_AST;
    } 
   
    if (tok.type == TOK_WHILE){
        AST* while_AST = (AST*)calloc(1, sizeof(AST));
        while_AST->type = AST_WHILE;
        eat(ps, TOK_WHILE);
        eat(ps, TOK_LPAREN);
        while_AST->cond = parse_expr(ps);
        eat(ps, TOK_RPAREN);
        eat(ps, TOK_LCURLY);
        while_AST->left = parse_statement(ps); // just like if just dont use the right, if while condition is not true, it just exit and return null 
        while_AST->right = NULL;
        eat(ps, TOK_RCURLY);
        
        return while_AST;
    }
        
    if (is_unOp(tok.type)){
        eat(ps, tok.type);
        AST* operand = parse_term(ps);
        AST* node = calloc(1, sizeof(AST));
        node->type = AST_UNARY;
        node->op = tok;
        node->right = operand;
        return node;
    }
    if (tok.type == TOK_LPAREN){
        eat(ps, TOK_LPAREN);
        AST* node = parse_expr(ps);
        eat(ps, TOK_RPAREN);
        return node;
    }
    if (tok.type == TOK_INT) {
        eat(ps, TOK_INT);
        AST *num = (AST*)calloc(1, sizeof(AST));
        num->type = AST_NUM;
        num->value = tok.value;
        return num;
    }
    if (tok.type == TOK_ID){
        eat(ps, TOK_ID);
        AST*var = make_id(tok.lexeme);
        return var; 
    }
    syntax_error("Failed at parse_term(). Token is not INT_LITERAL or ID", tok);
    return NULL;
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
        // this case AST_IF does not work as expected, always return the last value rather than returning the value that meet condition of if_statement
        // dont change this. GREAT CAUTION. WOMAN CAN KILL. 
        /*case AST_IF: {
            int cond = eval_ast_assignment(node->cond);
            int true_eval = eval_ast_assignment(node->left);
            if (node->right){
                int false_eval = eval_ast_assignment(node->right);
                return cond ? true_eval : false_eval;
            }
            return true_eval;
         }*/
        case AST_IF: int cond_value = eval_ast_assignment(node->cond);
                    if (cond_value) {
                        return eval_ast_assignment(node->left);
                    } else if (node->right) { // ERROR??  
                        return eval_ast_assignment(node->right);
                    } else { return 0; }
        case AST_WHILE: {
            const int MAX_ITERS = 1<<20;
            int iters = 0;
            int cond_val = eval_ast_assignment(node->cond);
            // Execute while the condition stays true
            while (cond_val != 0) {
                if (++iters > MAX_ITERS) {
                    fprintf(stderr, "WHILE: iteration limit reached\n");
                    break;
                }
                if (node->left) {
                    eval_ast_assignment(node->left);   // run body
                }
            }
            return 0;   // while is a statement; return whatever your language expects
        }
        // for is not even in the equation and already trying to be ambitious? I hate those kinds of people.
        // finish essential then move on to complex or looping ops. GREAT CAUTION. 
        /*case AST_ELIF: {
            int cond = eval_ast_assignment(node->cond);
            int true_eval = eval_ast_assignment(node->left);
            if (node->right){
                int false_eval = eval_ast_assignment(node->right);
                return cond ? true_eval : false_eval;
            }
            return true_eval;
        } */ 
        // do not need to create more AST type, the simpler the better, do not create more AST_type. 
        case AST_ELSE: return eval_ast_assignment(node->left); 
        case AST_BINOP: {
            int left = eval_ast_assignment(node->left);
            int right = eval_ast_assignment(node->right);
            return eval_binary(node->op.type, left, right);
        }
        case AST_ASSIGN: {                      // handle assignment here
            int val = eval_ast_assignment(node->right);
            int *slot = slot_for(node->left->name);   // lhs must be an identifier name
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

// prototypes printing tree 
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
        case AST_IF: {int cond = eval_ast_assignment(node->cond);
        sprintf(wrapper->label, "IF(%s)", cond ? "TRUE" : "FALSE");
        if (cond) {
        wrapper->left  = build_wrapper_tree(node->left);  // THEN
        wrapper->right = NULL;                            // skip ELSE
        } else {
            wrapper->left  = build_wrapper_tree(node->right); // ELSE
            wrapper->right = NULL;                            // skip THEN
        }

        if (wrapper->left)
            wrapper->left->parent_dir = -1;

        break;} 
        // while is just similar to IF, if we could categorize them into one AST type and then distinguish the printing by just the node->op.type
        case AST_WHILE: {int cond = eval_ast_assignment(node->cond);
        sprintf(wrapper->label, "WHILE (%s)", cond ? "TRUE" : "FALSE");
        if (cond) {
        wrapper->left  = build_wrapper_tree(node->left);  // THEN
        wrapper->right = NULL;                            // skip ELSE
        } else {
            wrapper->left  = build_wrapper_tree(node->right); // ELSE
            wrapper->right = NULL;                            // skip THEN
        }

        if (wrapper->left)
            wrapper->left->parent_dir = -1;

        break;} 
        case AST_ELIF: sprintf(wrapper->label, "ELSE IF(%s)", node->op.lexeme); break;
        case AST_ELSE: sprintf(wrapper->label, "ELSE(%s)", node->op.lexeme); break;
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
