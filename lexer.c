// C program to illustrate the implementation of lexical
// analyser
//source: geekforgeek => need to rewrite my own version
#include "lexer.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
typedef struct {
    const char *input;
    size_t pos;
    char current;
} Lexer; */

// this function moves the lexer one character forward in the input string
static void advance(Lexer *lex) {
    if (lex->input[lex->pos] == '\0') { // keep track of the current index
        lex->current = '\0'; // hold the current index being examined
        return;
    }
    lex->pos++; // advance to the next posistion on the string
    lex->current = lex->input[lex->pos];
}
// this function is to skip over any whitespace 
static void skip_whitespace(Lexer *lex) {
    while (lex->current && isspace((unsigned char)lex->current)) {
        advance(lex);
    }
}
// this function to generate token or tokenize
static Token token_gen(TokenType t, int v, const char *lex, size_t p) {
    Token tok = {.type = t, .value = v,.lexeme = {0}, .pos = p}; // generate token type, value and position
    if (lex && *lex) {
        strncpy(tok.lexeme, lex, sizeof(tok.lexeme) - 1);
    }
    return tok;
}
static Token token_gen2(TokenType t, int v, const char *lex, size_t p, size_t id_no) {
    Token tok = {.type = t, .value = v,.lexeme = {0}, .pos = p, .id_no = id_no}; // generate token type, value and position
    if (lex && *lex) {
        strncpy(tok.lexeme, lex, sizeof(tok.lexeme) - 1);
    }
    return tok;
}
// this function to read number from the input string then return it as a token
static Token number(Lexer *lex) {
    size_t start = lex->pos;
    long val = 0;
    char buf[64] = {0};
    while (lex->current && isdigit(lex->current)) // loop as long as the end of the string is reached and if it is a digit
    {
        val = val * 10 + (lex->current - '0'); // multiply the previous digits to the left and add the current digit. 
        advance(lex);
    }
    return token_gen(TOK_INT, (int)val, buf ,start);
}
// this function is to put together all the keyword so if later i want to add more keyword, make them auto.
int is_key (char *str) 
{
    const char* keyword[] = {"if", "else", "while","do", "break","return", "for"};
     for (int i = 0; i < sizeof(keyword) / sizeof(keyword[0]); i++) {
        if (strcmp(str, keyword[i]) == 0) {
            return 1;  // return true if the keywords are int, return, if 
        }
    }
    return 0;
}

int is_elif(char* buf, size_t* buf_idx, Lexer* lex){
    size_t buf_idx_start = *buf_idx;
    size_t lex_idx_start = lex->pos;
    if (isspace((unsigned char) lex->current)){
        buf[(*buf_idx)++] = ' ';  
        advance(lex);
    } else return 0;
    while(isalpha((unsigned char) lex->current) && *buf_idx < (size_t) (sizeof(buf) - 1)){
        buf[(*buf_idx)++] = lex->current;
        advance(lex);
    }
    buf[*buf_idx] = '\0';
    if (strcmp(buf, "else if") == 0){
        return 1;
    } else{
        buf[buf_idx_start] = '\0';
        *buf_idx = buf_idx_start;
        lex->pos = lex_idx_start;
        lex->current = lex->input[lex->pos];
        return 0;
    }
}
// this function to find identifier and return it as token. 
static Token identifier(Lexer *lex) {
    size_t start= lex->pos;
    size_t i = 0;
    char buf[64];
    while (lex->current && (isalpha((unsigned char)lex->current) || lex->current == '_' )) {
        if (i < sizeof(buf) - 1)
            buf[i++] = lex->current;
            advance(lex);
    }
    buf[i] = '\0';
    if (strcmp(buf, "while") == 0){ return token_gen(TOK_WHILE, 0, buf, start);} 
    else if (strcmp(buf, "return") == 0){ return token_gen(TOK_RETURN, 0, buf, start);} 
    else if (strcmp(buf, "int") == 0){ return token_gen(TOK_INT_VAR, 0, buf , start);}
    else if (strcmp(buf, "if") == 0){ return token_gen(TOK_IF, 0, buf, start);}
    else if (strcmp(buf, "else") == 0){
        if (is_elif(buf, &i, lex)){
            return token_gen(TOK_ELIF, 0, buf, start);
        }
        return token_gen(TOK_ELSE, 0, buf, start);}
    return token_gen2(TOK_ID, 0, buf, start, lex->id_cnt++);
}
void lexer_init(Lexer *lex, char *input) {
    lex->input = input;
    lex->pos = 0;
    lex->current = input && input[0] ? input[0] : '\0';
    lex->id_cnt = 0;
}
// consider next token in the input stream 
Token lexer_next_token(Lexer *lex) {
    while (lex->current) {
        // checking if white space
        if (isspace((unsigned char)lex->current)) {
            skip_whitespace(lex);
            continue;
        }
        // checking if number
        if (isdigit((unsigned char)lex->current)) {
            return number(lex);
        }
        //checking if identifiers.
        if (isalpha((unsigned char)lex->current) || lex->current == '_') {
            return identifier(lex);
        }
        
        size_t p = lex->pos;
        char ch = lex->current;
        advance(lex);
        // lord, this is for single char operations, please do double char later. -Thuong 
        switch (ch) {
            case '+': if (lex->current == '+') {advance(lex); return token_gen(TOK_INCREMENT, 0, "++", p);}
                      else if (lex->current == '=') {advance(lex); return token_gen(TOK_COMPOUND_PLUS, 0, "+=", p);}
                      else { return token_gen(TOK_PLUS, 0, "+" , p); }
            case '-': if (lex->current == '-') { advance(lex); return token_gen(TOK_DECREMENT, 0, "--", p); }
                      else if (lex->current == '=') { advance(lex); return token_gen(TOK_COMPOUND_MINUS, 0, "-=", p); }
                      else {return token_gen(TOK_MINUS, 0,"-", p);}
            case '*': if (lex->current == '=') { advance(lex); return token_gen(TOK_COMPOUND_MUL, 0, "*=", p); } 
                      return token_gen(TOK_MUL, 0, "*",p);
            case '/': if (lex->current == '=') { advance(lex); return token_gen(TOK_COMPOUND_DIV, 0, "/=", p); }
                      return token_gen(TOK_DIV, 0, "/",p);
            case '(': return token_gen(TOK_LPAREN, 0,"(", p);
            case ')': return token_gen(TOK_RPAREN, 0, ")",p);
            case '{': return token_gen(TOK_LCURLY, 0,"(", p);
            case '}': return token_gen(TOK_RCURLY, 0, ")",p);
            case '=': if (lex->current == '=') { advance(lex); return token_gen(TOK_EQ, 0,"==" ,p); }
                      else {return token_gen(TOK_ASSIGN, 0,"=" ,p);}
            case '>': if (lex->current == '>') { advance(lex); return token_gen(TOK_SHIFT_RIGHT, 0,">>" ,p); }
                      else if (lex->current == '=') {advance(lex); return token_gen(TOK_GREATER_EQ, 0, ">=", p);}
                      else {return token_gen(TOK_GREATER, 0,">" ,p);}
            case '<': if (lex->current == '<') { advance(lex); return token_gen(TOK_SHIFT_LEFT, 0,"<<" ,p); }
                      else if (lex->current == '=') {advance(lex); return token_gen(TOK_LESS_EQ, 0, "<=", p);}
                      else {return token_gen(TOK_LESS, 0,"<" ,p);}
            case '&': if (lex->current == '&') { advance(lex); return token_gen(TOK_AND, 0,"&&" ,p); }
                      else {return token_gen(TOK_BITWISE_AND, 0,"&" ,p);}
            case '^': return token_gen(TOK_BITWISE_XOR, 0,"^" ,p); 
            case '~': return token_gen(TOK_BITWISE_NOT, 0,"~" ,p); 
            //case '?': return token_gen(TOK_QUESTION, 0,"?" ,p);
            //case ':': return token_gen(TOK_COLON, 0,":" ,p);
            case '!': if (lex->current == '=') { advance(lex); return token_gen(TOK_NOT_EQ, 0,"!=" ,p); }
                      else {return token_gen(TOK_NOT, 0,"!" ,p);}      
            case '|': if (lex->current == '|') { advance(lex); return token_gen(TOK_OR, 0,"||" ,p); }
                      else {return token_gen(TOK_BITWISE_OR, 0,"|" ,p);}    
            case ';': return token_gen(TOK_SEMI, 0, ";", p);
            default:  // unknown char: consume until end; caller can treat as END
             
                return token_gen(TOK_EOF, 0, "EOF",p);
        }
    }
    return token_gen(TOK_EOF, 0, "EOF",lex->pos);
}
