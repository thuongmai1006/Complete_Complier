#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Target.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Transforms/Scalar.h>
#include "codegen.h"

named_value *named_values = NULL;

static LLVMContextRef TheContext;
static LLVMModuleRef TheModule;
static LLVMBuilderRef TheBuilder;

static void InitializeModule(){
    TheContext = LLVMContextCreate();
    TheModule = LLVMModuleCreateWithNameInContext("JIT", TheContext);
    TheBuilder = LLVMCreateBuilderInContext(TheContext);
}

LLVMValueRef codegen_number(AST* node){
    if (node == NULL) {
        return NULL;
    }
    
    switch (node->op.type){
        case TOK_INT: 
            return LLVMConstInt(LLVMInt32Type(),(unsigned long long)node->value, 0);
        case TOK_FLOAT: 
            return LLVMConstReal(LLVMFloatType(), (double)node->value);
        case TOK_DBL: 
            return LLVMConstReal(LLVMDoubleType(), (double) node->value);
        default: {
            return NULL;
        }
    }
}

LLVMValueRef codegen_binary_expr(AST* node, LLVMModuleRef module, LLVMBuilderRef builder){
    if (node == NULL) {
        return NULL;
    }
    
    LLVMValueRef lhs = codegen_number(node->left);
    LLVMValueRef rhs = codegen_number(node->right);
    if (lhs == NULL || rhs == NULL){
        return NULL;
    }

    switch(node->op.type){
        case TOK_PLUS:
            return LLVMBuildAdd(builder, lhs, rhs, "addtmp");
        case TOK_MINUS:
            return LLVMBuildSub(builder, lhs, rhs, "subtmp");
        case TOK_MUL:
            return LLVMBuildMul(builder, lhs, rhs, "multmp");
        case TOK_DIV:
            return LLVMBuildFDiv(builder, lhs, rhs, "divtmp");
        default: 
            return NULL;
    }
}

void codegen(AST* node){
    fprintf(stderr, "\n========== CODEGEN START ==========\n");
    
    if (node == NULL) {
        return;
    }
    InitializeModule();
    LLVMValueRef result = codegen_binary_expr(node, TheModule, TheBuilder);
    
    if (result) {
        LLVMDumpModule(TheModule);
    } else {
        fprintf(stderr, "ERROR: codegen returned NULL\n");
    }
    
    fprintf(stderr, "========== CODEGEN END ==========\n\n");
}
