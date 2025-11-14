#include <stdlib.h>
#include <string.h>
#include "syntax.h"
#include <llvm-c/Core.h>

typedef struct named_value {
    const char* name;
    LLVMValueRef value;
} named_value;


void codegen(AST*);
