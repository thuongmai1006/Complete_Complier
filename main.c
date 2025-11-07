#include <stdio.h>
#include <string.h>
#include "lexer.h"
#include "syntax.h"
static const char* token_type_name(TokenType t) {
    switch (t) {
        case TOK_INT: return "INTEGER";
        case TOK_ID:      return "ID";
        case TOK_ASSIGN:    return "ASSIGNMENT";
        case TOK_EQ:    return "EQUAL";
        case TOK_PLUS:    return "PLUS";
        case TOK_MINUS:   return "MINUS";
        case TOK_MUL:     return "MUL";
        case TOK_DIV:     return "DIV";
        case TOK_LPAREN:  return "Left Parenthese";
        case TOK_RPAREN:  return "Right Parenthese";
        case TOK_EOF:     return "END OF STRING";
        default:          return "?";
    }
}
static void print_token(const Token *tk) {
    if (tk->type == TOK_INT)
        printf("Token=> Catergory:%s, value=%d, lexeme=\"%s\", pos=%zu\n",
               token_type_name(tk->type), tk->value, tk->lexeme, tk->pos);
    else if (tk->type == TOK_ID)
        printf("Token=> Catergory:%s, value=%d, lexeme=\"%s\", pos=%zu\n",
               token_type_name(tk->type),tk->value, tk->lexeme, tk->pos);
    else
        printf("Token=> Category:%s, lexeme=\"%s\", pos=%zu\n",
               token_type_name(tk->type), tk->lexeme, tk->pos);
}
void dump_tokens(const char *examples) {
    Lexer lx; lexer_init(&lx, examples);
    for (;;) {
        Token tk = lexer_next_token(&lx);
        print_token(&tk);
        if (tk.type == TOK_EOF) break;
    }
}
int main(void) {
    const char *examples[] = {
        "x = 2 + 3",
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

        AST *tree = parse_expr(&ps);
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
        Parser ps;
        parser_init(&ps, &lx);

        AST *tree = parse_expr(&ps);
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
