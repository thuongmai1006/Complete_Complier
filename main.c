#include <stdio.h>
#include <string.h>
#include "symbol_table.h"
#include "lexer.h"
#include "syntax.h"
#include <stdbool.h>
// switch-based lookup-> MAPPING ENUM-> STRING
// No extra storage. O(1) expected lookup using hash function
// Stores key/value pairs dynamically
static const char *token_type_name(TokenType t)
{
    switch (t)
    {
    case TOK_INT:
        return "int";
    case TOK_FLOAT:
        return "f";
    case TOK_ID:
        return "ID";
    case TOK_ASSIGN:
        return "=";
    case TOK_EQ:
        return "==";
    case TOK_PLUS:
        return "+";
    case TOK_MINUS:
        return "-";
    case TOK_MUL:
        return "*";
    case TOK_DIV:
        return "/";
    case TOK_LPAREN:
        return "(";
    case TOK_RPAREN:
        return ")";
    case TOK_LCURLY:
        return "{";
    case TOK_RCURLY:
        return "}";
    case TOK_EOF:
        return "End of File";
    case TOK_OR:
        return "||";
    case TOK_BITWISE_XOR:
        return "^";
    case TOK_BITWISE_OR:
        return "|";
    case TOK_AND:
        return "&&";
    case TOK_BITWISE_AND:
        return "&";
    case TOK_NOT_EQ:
        return "!=";
    case TOK_GREATER:
        return ">"; // <<
    case TOK_LESS:
        return "<"; // >>
    case TOK_GREATER_EQ:
        return ">="; // <<
    case TOK_LESS_EQ:
        return "<="; // >>
    case TOK_SHIFT_LEFT:
        return "<<"; // <<
    case TOK_SHIFT_RIGHT:
        return ">>"; // >>
    case TOK_IF:
        return "if";
    case TOK_ELSE:
        return "else";
    case TOK_BREAK:
        return "break";
    case TOK_WHILE:
        return "while";
    case TOK_FOR:
        return "for";
    case TOK_DO:
        return "do";
    case TOK_RETURN:
        return "return";
    case TOK_INT_VAR:
        return "int";
    case TOK_FLOAT_VAR:
        return "float";
    case TOK_SEMI:
        return ";";
    case TOK_COMMA:
        return ",";
    case TOK_ELIF:
        return "else if";
    case TOK_VOID:
        return "void";
    default:
        return "?";
    }
}
// DEBUG-------------------------------------------------------------------------------------------------
// eveluation function accuracy (like void)=> mostly to debug and verify
static double eval_function_call(AST *fn, AST **args, int arg_count)
{
    if (!fn || fn->type != AST_FUNC)
    {
        printf("ERROR: Not a function node!\n");
        return 0.0;
    }

    printf("Starting function execution, stmnt_cnt=%d, param_count=%d\n",
           fn->stmnt_cnt, fn->param_cnt);

    int param_count = fn->param_cnt;

    printf("Registering %d parameters\n", param_count);

    // Register parameters
    for (int i = 0; i < param_count; i++)
    {
        printf("Registering parameter %d\n", i);
        eval_ast_decl(fn->params[i]);
    }

    // Execute body statements
    double return_val = 0.0;
    int has_returned = 0;

    printf("Executing %d body statements", fn->stmnt_cnt);

    for (int i = 0; i < fn->stmnt_cnt && !has_returned; i++)
    {
        if (!fn->stmnts[i])
        {
            printf("Statement %d is NULL, skipping\n", i);
            continue;
        }

        printf("Executing statement %d, type=%d\n", i, fn->stmnts[i]->type);

        if (fn->stmnts[i]->type == AST_RETURN)
        {
            if (fn->stmnts[i]->stmnt_cnt > 0 && fn->stmnts[i]->stmnts[0])
            {
                return_val = eval_ast_assignment(fn->stmnts[i]->stmnts[0]);
            }
            else
            {
                return_val = 0.0;
            }
            has_returned = 1;
            printf("Function returning: %.2f\n", return_val);
        }
        else
        {
            eval_ast_assignment(fn->stmnts[i]);
        }
    }

    printf("Function execution complete\n");
    return return_val;
}
// to decide parse_fn or parse_statement (like calculator without function delc)--------------
bool is_function_definition(Parser *ps)
{
    // Check if we have: type identifier '('

    // First token must be a type
    if (ps->current.type != TOK_VOID &&
        ps->current.type != TOK_INT_VAR && // Use TOK_INT_VAR, not TOK_INT
        ps->current.type != TOK_FLOAT_VAR)
    { // Use TOK_FLOAT_VAR, not TOK_FLOAT
        return false;
    }

    // Second token must be an identifier
    if (ps->next.type != TOK_ID)
    {
        return false;
    }

    // Save state to peek at third token
    size_t saved_pos = ps->lexer->pos;
    char saved_char = ps->lexer->current;
    Token saved_current = ps->current;
    Token saved_next = ps->next;

    // Advance twice to see the third token
    advance(ps); // Move to ID
    advance(ps); // Move to what's after ID
    bool is_func = (ps->current.type == TOK_LPAREN);
    // Restore state
    ps->current = saved_current;
    ps->next = saved_next;
    ps->lexer->pos = saved_pos;
    ps->lexer->current = saved_char;

    return is_func;
}
// PRINT token class ----------------------------------------------------------
static void print_token_classification(Lexer *lx)
{
    // Reset lexer to beginning
    lexer_init(lx, lx->input);
    Token tk;
    while ((tk = lexer_next_token(lx)).type != TOK_EOF)
    {
        if (tk.type == TOK_ID)
        {
            printf("Identifier: %s pos %zu\n", tk.lexeme, tk.pos);
        }
        else if (tk.type == TOK_INT_VAR || tk.type == TOK_FLOAT_VAR ||
                 tk.type == TOK_VOID || tk.type == TOK_IF ||
                 tk.type == TOK_ELSE || tk.type == TOK_ELIF ||
                 tk.type == TOK_FOR || tk.type == TOK_WHILE ||
                 tk.type == TOK_DO || tk.type == TOK_RETURN ||
                 tk.type == TOK_BREAK)
        {
            printf("Keyword: '%s' at pos -> %zu\n", tk.lexeme, tk.pos);
        }
        else if (tk.type == TOK_INT || tk.type == TOK_FLOAT)
        {
            printf("Number: '%s' at pos -> %zu\n", tk.lexeme, tk.pos);
        }
        else if (tk.type == TOK_ASSIGN ||
                 tk.type == TOK_PLUS || tk.type == TOK_MINUS ||
                 tk.type == TOK_MUL || tk.type == TOK_DIV ||
                 tk.type == TOK_EQ || tk.type == TOK_NOT_EQ ||
                 tk.type == TOK_LESS || tk.type == TOK_GREATER ||
                 tk.type == TOK_LESS_EQ || tk.type == TOK_GREATER_EQ ||
                 tk.type == TOK_AND || tk.type == TOK_OR ||
                 tk.type == TOK_BITWISE_AND || tk.type == TOK_BITWISE_OR ||
                 tk.type == TOK_BITWISE_XOR ||
                 tk.type == TOK_SHIFT_LEFT || tk.type == TOK_SHIFT_RIGHT ||
                 tk.type == TOK_INCREMENT || tk.type == TOK_DECREMENT)
        {
            printf("Operator: '%s' at pos -> %zu\n", tk.lexeme, tk.pos);
        }
        else if (tk.type == TOK_LPAREN || tk.type == TOK_RPAREN ||
                 tk.type == TOK_LCURLY || tk.type == TOK_RCURLY ||
                 tk.type == TOK_SEMI || tk.type == TOK_COMMA)
        {
            printf("Punctuation: '%s' at pos -> %zu\n", tk.lexeme, tk.pos);
        }
    }
}
// PRINT token --------------------------------------
static void print_token(const Token *tk)
{
    if (tk->type == TOK_INT)
        printf(" %s(%d) ",
               token_type_name(tk->type), tk->value);
    else if (tk->type == TOK_ID)
        printf(" <%s,%zu> ",
               token_type_name(tk->type), tk->id_no);
    else if (tk->type == TOK_EOF)
        printf("\n\n %s at pos=%zu, \"%s\"\n ",
               token_type_name(tk->type), tk->pos, tk->lexeme);
    else
        printf(" <%s> ",
               token_type_name(tk->type));
}
// PRINT TOKENS from input streams --------------------------------------
void print_tokens(Lexer *lx)
{
    size_t r_pos = lx->pos;
    char r_char = lx->current;
    while (1)
    {
        Token tk = lexer_next_token(lx);
        print_token(&tk);
        if (tk.type == TOK_EOF)
        {
            lx->pos = r_pos;
            lx->current = r_char;
            break;
        }
    }
}
// PRINT --------------------------------------
static void print_sep()
{
    for (int i = 0; i < 80; ++i)
    {
        putchar('-');
    }
    putchar('\n');
}


int main(void) {
	char *examples[] = {
		/*"(1 + 2) * 3 - 4 / 2;",
		   "int x=1;",
		   "!x;",
		   "int X=20;",
		   "X++;",
		   "int Y=0;",
		   "int Z=4;",
		   "int K=5;",
		   "X = Y + Z*3 + K/2; ",
		   "(1 + 2) * 3 - 4 / 2;",
		   "int X =20; if (X>23) { I=0;} else {I=1;}",
		   "if (X>2) {U=9;} else {U=0;} ",
		   "int x = 5;\nif (x > 23) {int y = 1;}\n else if(x>2) {int z = 2;} \n else {z=1;}\n",*/
		"int sum=0;\nfor (int i=0; i<10; i++)\n{sum=sum+i;};\n ",
		//"float a=18.5;",
		"int add (int a, int b){\n\tint sum=0;\n\tfor (int i=0; i<10; i++){\n\t\tsum=sum+i;\n\t}\n return sum;\n}",
		//" int main () { int sum=0; for (int i=0; i<10; i++) {sum=sum+i;}; \n return sum; }",
		"int sum=0; for (int i=0; i<10; i++) {sum=sum+i;}",
		"3+3;",
		"int sum=0; for (int i=0; i<10; i++) {sum=sum+i;}",
		"int sum(int a, int b, int c, float d){\n\ta + b + c;\n\t3+3;\n\treturn a + b;\n}",
        "int choice(int a, int b, int choice) { for (int i = 0; i < 10; i++) { a + b; }\n if (a + b) { return a;} else {return b;}}",
        //"int fib(int x){if (x < 3) {return 1;} else {return fib(x-1) + fib(x-2)};}",
//		"if (X>23) { I=0;} else {I=1;}",
//		"if (X>23) {K = 20;}\nelse{K= 10;}\nint x = 5;\n3+4;",
        "3 + 4 * 5;",
		NULL
	};

    for (int i = 0; examples[i]; ++i)
    {
        Lexer lx;
        print_sep();
        puts("\n=============INPUT STREAM (TEXT)===============================");
        printf("%s\n", examples[i]);
        lexer_init(&lx, examples[i]);
        puts("\n============TOKEN STREAM (TOKENIZE)============================");
        print_tokens(&lx);
        lexer_init(&lx, examples[i]);
        puts("\n=============TOKEN CLASSIFICATIONS (TokenType)=================");
        print_token_classification(&lx);
        print_sep();
        lexer_init(&lx, examples[i]);
        Parser ps;
        parser_init(&ps, &lx);
        while (ps.current.type != TOK_EOF)
        {
            AST *tree = NULL;
            if (is_function_definition(&ps))
            {
                tree = parse_fn(&ps);
                if (tree && tree->type == AST_FUNC)
                {
                    puts("\n=================PARSE TREE (AST)===============================");
                    print_tree_better(tree);
                    printf("\n=============Executing function==============================\n");
                    double result = eval_function_call(tree, NULL, 0);
                    printf("Function returned: %.2f\n", result);
                    codegen_run(tree);
                    print_sep();
                }
            }
            else
            {
                tree = parse_statement(&ps);
                if (tree)
                {
                    double result = eval_ast_assignment(tree);
                    printf("result = %.2f\n", result);
                    puts("\n=============Parse tree==========================================");
                    print_tree_better(tree);
                }
            }
            print_sep();
            if (tree)
                free_ast(tree);
        }
    }
    // REPL (optional) TEST ON SCREEN
    char buf[1024];
    while (1)
    {
        printf("> ");
        if (!fgets(buf, sizeof(buf), stdin))
            break;
        // trim newline
        buf[strcspn(buf, "\n")] = '\0';
        if (buf[0] == '\0')
            continue;

        Lexer lx;
        lexer_init(&lx, buf);
        print_tokens(&lx);

        Parser ps;
        parser_init(&ps, &lx);

        AST *tree = NULL;

        // Check if this is a function definition
        if (is_function_definition(&ps))
        {
            tree = parse_fn(&ps);
        }
        else
        {
            tree = parse_statement(&ps);
        }

        if (!tree)
        {
            fprintf(stderr, "Parse failed\n");
            continue;
        }

        if (ps.current.type != TOK_EOF)
        {
            fprintf(stderr, "Trailing input at position %zu\n", ps.current.pos);
            free_ast(tree);
            continue;
        }
        // Only evaluate if it's a statement, not a function definition
        if (tree->type == AST_FUNC)
        {
            puts("== Parse tree up 1 ==");
            print_tree_better(tree);

            // ADD THIS:
            printf("\n=== Executing function ===\n");
            double result = eval_function_call(tree, NULL, 0);
            printf("Function returned: %.2f\n", result);
        }
        else
        {
            // Only evaluate non-function statements
            double result = eval_ast_assignment(tree);
            printf(" = %.2f\n", result);
            puts("== Parse tree down eher ==");
            print_tree_better(tree);
        }
        codegen_run(tree);
        free_ast(tree);
    }
    return 0;
}
