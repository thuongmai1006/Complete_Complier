// expr_demo.c
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* ===================== Tokens ===================== */
typedef enum {
  TOK_INT, TOK_ID, TOK_PLUS, TOK_MINUS, TOK_TIMES, TOK_DIVIDE,
  TOK_LPAREN, TOK_RPAREN, TOK_EOF
} TokenType;

typedef struct {
  TokenType type;
  union { int int_val; char *id_name; } value;
} Token;

typedef struct { Token *a; int n, cap; } TokVec;

static void tv_push(TokVec *tv, Token t){
  if (tv->n == tv->cap){ tv->cap = tv->cap? tv->cap*2:32; tv->a = realloc(tv->a, tv->cap*sizeof(Token)); }
  tv->a[tv->n++] = t;
}

static void free_tokens(TokVec *tv){
  for (int i=0;i<tv->n;i++) if (tv->a[i].type==TOK_ID) free(tv->a[i].value.id_name);
  free(tv->a);
}

/* ===================== Lexer ===================== */
static const char *SRC; static int I;

static void skip_ws(void){ while (SRC[I] && isspace((unsigned char)SRC[I])) I++; }

static Token make(TokenType t){ Token x; x.type=t; return x; }

static Token lex_int(void){
  long v=0; while (isdigit((unsigned char)SRC[I])){ v = v*10 + (SRC[I]-'0'); I++; }
  Token t = make(TOK_INT); t.value.int_val = (int)v; return t;
}

static Token lex_id(void){
  int s=I; I++; while (isalnum((unsigned char)SRC[I]) || SRC[I]=='_') I++;
  int n = I - s; char *name = malloc(n+1); memcpy(name,SRC+s,n); name[n]='\0';
  Token t = make(TOK_ID); t.value.id_name = name; return t;
}

static void lex_all(const char *input, TokVec *out){
  SRC = input; I = 0; skip_ws();
  for(;;){
    int c = SRC[I];
    if (!c){ tv_push(out, make(TOK_EOF)); break; }

    if (isdigit((unsigned char)c)){ tv_push(out, lex_int()); skip_ws(); continue; }
    if (isalpha((unsigned char)c) || c=='_'){ tv_push(out, lex_id()); skip_ws(); continue; }

    I++;
    switch (c){
      case '+': tv_push(out, make(TOK_PLUS));   break;
      case '-': tv_push(out, make(TOK_MINUS));  break;
      case '*': tv_push(out, make(TOK_TIMES));  break;
      case '/': tv_push(out, make(TOK_DIVIDE)); break;
      case '(': tv_push(out, make(TOK_LPAREN)); break;
      case ')': tv_push(out, make(TOK_RPAREN)); break;
      default: fprintf(stderr,"[lexer] skipping unknown char '%c'\n", c); break;
    }
    skip_ws();
  }
}

static const char* tname(TokenType t){
  switch(t){
    case TOK_INT: return "INT";
    case TOK_ID: return "ID";
    case TOK_PLUS: return "PLUS";
    case TOK_MINUS: return "MINUS";
    case TOK_TIMES: return "TIMES";
    case TOK_DIVIDE: return "DIVIDE";
    case TOK_LPAREN: return "LPAREN";
    case TOK_RPAREN: return "RPAREN";
    case TOK_EOF: return "EOF";
  } return "?";
}

static void print_tokens(const TokVec *tv){
  puts("== Lexical analysis (tokens) ==");
  for (int i=0;i<tv->n;i++){
    Token t = tv->a[i];
    if (t.type==TOK_INT)   printf("[%s:%d]\n", tname(t.type), t.value.int_val);
    else if (t.type==TOK_ID) printf("[%s:%s]\n", tname(t.type), t.value.id_name);
    else printf("[%s]\n", tname(t.type));
  }
}

/* ===================== Parser & AST ===================== */
/* Grammar:
   E -> T { ('+'|'-') T }
   T -> F { ('*'|'/') F }
   F -> '(' E ')' | INT | ID
*/

typedef enum { N_NUM, N_ID, N_BIN } NodeKind;
typedef struct Node {
  NodeKind kind; char op; struct Node *lhs,*rhs; char *text;
} Node;

static Node* node_bin(char op, Node* l, Node* r){
  Node* n = calloc(1,sizeof(Node)); n->kind=N_BIN; n->op=op; n->lhs=l; n->rhs=r; return n;
}
static Node* node_num(const char* s){ Node* n=calloc(1,sizeof(Node)); n->kind=N_NUM; n->text=strdup(s); return n; }
static Node* node_id (const char* s){ Node* n=calloc(1,sizeof(Node)); n->kind=N_ID;  n->text=strdup(s); return n; }

typedef struct { Token *a; int n; int i; } Stream;
static Stream S;

static Token* cur(void){ return &S.a[S.i]; }
static int accept(TokenType t){ if (cur()->type==t){ S.i++; return 1; } return 0; }
static void expect(TokenType t, const char* msg){ if (!accept(t)){ fprintf(stderr,"[parse] expected %s (%s)\n", tname(t), msg); exit(1);} }

static Node* parse_E(void); static Node* parse_T(void); static Node* parse_F(void);

static Node* parse_E(void){
  Node* n = parse_T();
  while (cur()->type==TOK_PLUS || cur()->type==TOK_MINUS){
    char op = (cur()->type==TOK_PLUS)? '+':'-';
    S.i++;
    Node* r = parse_T();
    n = node_bin(op, n, r);    // left associative
  }
  return n;
}

static Node* parse_T(void){
  Node* n = parse_F();
  while (cur()->type==TOK_TIMES || cur()->type==TOK_DIVIDE){
    char op = (cur()->type==TOK_TIMES)? '*':'/';
    S.i++;
    Node* r = parse_F();
    n = node_bin(op, n, r);    // left associative
  }
  return n;
}

static Node* parse_F(void){
  if (accept(TOK_LPAREN)){ Node* e = parse_E(); expect(TOK_RPAREN, "')'"); return e; }
  if (cur()->type==TOK_INT){ char buf[64]; snprintf(buf,sizeof(buf),"%d", cur()->value.int_val); S.i++; return node_num(buf); }
  if (cur()->type==TOK_ID){ Node* n = node_id(cur()->value.id_name); S.i++; return n; }
  fprintf(stderr,"[parse] expected '(', INT, or ID\n"); exit(1);
}

static void print_tree_ascii(const Node* n, const char* indent, int last){
  printf("%s%s", indent, last ? "`-- " : "|-- ");
  switch(n->kind){
    case N_NUM: printf("NUM(%s)\n", n->text); break;
    case N_ID:  printf("ID(%s)\n",  n->text); break;
    case N_BIN: printf("BIN('%c')\n", n->op); break;
  }
  char next[256]; snprintf(next,sizeof(next), "%s%s", indent, last ? "    " : "|   ");
  if (n->kind==N_BIN){
    print_tree_ascii(n->lhs, next, 0);
    print_tree_ascii(n->rhs, next, 1);
  }
}

static void free_tree(Node* n){
  if (!n) return;
  if (n->kind==N_BIN){ free_tree(n->lhs); free_tree(n->rhs); }
  else free(n->text);
  free(n);
}

/* ===================== Driver ===================== */
int main(void){
  char buf[1024];
  printf("Enter expression: ");
  if (!fgets(buf, sizeof buf, stdin)) return 1;
  buf[strcspn(buf,"\n")] = '\0';

  TokVec tv = {0};
  lex_all(buf, &tv);
  print_tokens(&tv);

  S.a = tv.a; S.n = tv.n; S.i = 0;
  Node* root = parse_E();
  if (cur()->type != TOK_EOF) { fprintf(stderr,"[parse] extra input remains\n"); free_tree(root); free_tokens(&tv); return 1; }

  puts("== Parse tree ==");
  print_tree_ascii(root, "", 1);

  free_tree(root);
  free_tokens(&tv);
  return 0;
}
