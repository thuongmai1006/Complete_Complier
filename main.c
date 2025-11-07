#include <stdio.h>
#include <string.h>
#include "lexer.h"
#include "syntax.h"
static const char* token_type_name(TokenType t) {
    switch (t) {
        case TOK_INT: return "INT";
        case TOK_ID:      return "ID";
        case TOK_ASSIGN:    return "=";
        case TOK_EQ:    return "==";
        case TOK_PLUS:    return "+";
        case TOK_MINUS:   return "-";
        case TOK_MUL:     return "*";
        case TOK_DIV:     return "/";
        case TOK_LPAREN:  return "(";
        case TOK_RPAREN:  return ")";
        case TOK_EOF:     return "End of File";
        case TOK_OR:           return "||";
        case TOK_BITWISE_XOR:           return "^";
        case TOK_BITWISE_OR:   return "|";
        case TOK_AND:          return "&&";
        case TOK_BITWISE_AND:  return "&";
        case TOK_NOT_EQ:          return "!=";
        case TOK_SHIFT_LEFT:    return "<<";// <<
        case TOK_SHIFT_RIGHT:   return ">>"; // >>
        default:          return "?";
    }
}
/*static void print_token(const Token *tk) {
    if (tk->type == TOK_INT)
        printf("%s,(%d), lexeme=\"%s\", pos=%zu ",
               token_type_name(tk->type), tk->value, tk->lexeme,tk->pos);
    else if (tk->type == TOK_ID)
        printf("<%s,%zu>, lexeme=\"%s\" ",
               token_type_name(tk->type), tk->pos, tk->lexeme);
    else if (tk->type == TOK_EOF)
        printf("<%s,%zu>, lexeme=\"%s\"\n ",
               token_type_name(tk->type), tk->pos, tk->lexeme);
    else
        printf("<%s,%zu> lexeme=\"%s\" ",
               token_type_name(tk->type),tk->pos, tk->lexeme);
}*/
static void print_token(const Token *tk) {
    if (tk->type == TOK_INT)
        printf(" %s(%d) ",
               token_type_name(tk->type), tk->value);
    else if (tk->type == TOK_ID)
        printf(" <%s,%zu> ",
               token_type_name(tk->type), tk->pos);
    else if (tk->type == TOK_EOF)
        printf("\n %s at pos=%zu, lexeme=\"%s\"\n ",
               token_type_name(tk->type), tk->pos, tk->lexeme);
    else
        printf("<%s> ",
               token_type_name(tk->type));
        }
void dump_tokens(char *examples) {
    Lexer lx; lexer_init(&lx, examples);
    for (;;) {
        Token tk = lexer_next_token(&lx);
        print_token(&tk);
        if (tk.type == TOK_EOF) break;
    }
}
void dump_token_input(char *input) {  // madness right here * for print_token by string or by each buf[i]. Time wasted: 30'
    Lexer lx; lexer_init(&lx, input);
    for (;;) {
        Token tk = lexer_next_token(&lx);
        print_token(&tk);
        if (tk.type == TOK_EOF) break;
    }
}
int main(void) {
     char *examples[] = {
        "X = 3",
        "X++",
        "Y=0",
        "Z=4",
        "K=5",
        "X = Y + Z*3 + K/2 ",
        "!X ",
        "(1 + 2) * 3 - 4 / 2",
        "-5 + (10 - 3) * 2",
        NULL
    };

    for (int i = 0; examples[i]; ++i) {
        Lexer lx;
        lexer_init(&lx, examples[i]);
        dump_tokens(examples[i]);
        Parser ps;
        parser_init(&ps, &lx);

        AST *tree = parse_statement(&ps);
        if (ps.current.type != TOK_EOF) fprintf(stderr,"Trailing input at %zu\n", ps.current.pos);
        if (ps.current.type != TOK_EOF) {
            fprintf(stderr, "Trailing input at position %zu\n", ps.current.pos);
            free_ast(tree);
            return 1;
        }

        int result = eval_ast_assignment(tree);
        printf("%s = %d\n", examples[i], result);
        free_ast(tree);
    }

    // REPL (optional)
    char buf[1024];
    while (1) {
        printf("> ");
        if (!fgets(buf, sizeof(buf), stdin)) break;
        // trim newline
        buf[strcspn(buf, "\n")] = '\0';
        if (buf[0] == '\0') continue;
        
            Lexer lx;
            lexer_init(&lx, buf);
            dump_token_input(buf);
            Parser ps;
            parser_init(&ps, &lx);
         
            AST *tree = parse_statement(&ps);
            if (ps.current.type != TOK_EOF) {
                fprintf(stderr, "Trailing input at position %zu\n", ps.current.pos);
                free_ast(tree);
                continue;
            }
            int result = eval_ast_assignment(tree);
            printf("%d\n", result);
            free_ast(tree);
        
    }
    return 0;
}
