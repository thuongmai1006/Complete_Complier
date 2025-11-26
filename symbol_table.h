#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H
#include <llvm-c/Core.h>
#include "syntax.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define TABLE_SIZE 100
void codegen_run(AST*);

typedef struct Value {
    char *key;
    LLVMValueRef value;
    struct Value *next;
} Value;

typedef struct {
    Value *buckets[TABLE_SIZE];
} HashMap;

unsigned int hash(const char* key);
HashMap* create_map();
void map_put(HashMap *map, const char *key, LLVMValueRef value);
LLVMValueRef map_get(HashMap *map, const char *key);
void map_clear(HashMap *map);
#endif
