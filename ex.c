
static AST* parse_if (Parser *ps){
    AST* node = malloc(sizeof *node);
    eat (ps, TOK_IF);
    eat (ps, TOK_LPAREN); // not sure here -Thuong 
    AST *if_cond = parse_expr(ps); // parse the if condition inside the ()
    if (ps->current.type != TOK_RPAREN) { syntax_error("expected ')'", ps->current); }
    eat(ps, TOK_RPAREN);
    AST *then_branch = parse_statement(ps);
    AST *else_branch = NULL;
    //if (if_cond)  { return parse_block(ps); } // if condition is true then parse_block
    //return if_cond;
    if (ps->current.type == TOK_ELSE) {
        eat(ps, TOK_ELSE); 
        else_branch = parse_statement(ps);   // if else then keep parse 
    }
    node->type = AST_IF;
    node->cond = if_cond;
    node->left = then_branch;
    node->right = else_branch;
    return node;
}

