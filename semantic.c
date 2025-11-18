//symbol tables 
//typekind
typedef enum { TY_INT, TY_BOOL, TY_VOID, TY_INVALID } TypeId;

typedef struct Type { TypeId id; } Type;

typedef enum { SYM_VAR, SYM_FUNC } SymKind;

typedef struct Param {
  const char *name;
  Type       type;
  struct Param *next;
} Param;

typedef struct Symbol {
  const char *name;
  SymKind     kind;
  Type        type;        // for var: its type; for func: return type
  Param      *params;      // for func only
  int         is_defined;  // for funcs: has body?
  struct Symbol *next;     // chained in a hash bucket
} Symbol;

//scopes
#define NSLOTS 211

typedef struct Scope {
  Symbol *slots[NSLOTS];
  struct Scope *parent;
} Scope;

static Scope *scope_push(Scope *cur) {
  Scope *s = calloc(1, sizeof(Scope));
  s->parent = cur; return s;
}
static Scope *scope_pop(Scope *cur) { Scope *p = cur->parent; free(cur); return p; }

static unsigned h(const char *s){ unsigned v=1469598101u; for(;*s;++s) v=(v^*s)*16777619u; return v; }
static Symbol *scope_lookup(Scope *s, const char *name){
  for (; s; s = s->parent){
    Symbol *p = s->slots[h(name)%NSLOTS];
    for (; p; p=p->next) if (!strcmp(p->name,name)) return p;
  }
  return NULL;
}
static Symbol *scope_insert(Scope *s, Symbol *sym){
  unsigned i = h(sym->name)%NSLOTS; sym->next = s->slots[i]; s->slots[i] = sym; return sym;
}
