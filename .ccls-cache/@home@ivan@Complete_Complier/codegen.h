#include <stdlib.h>
#include <string.h>
#include "syntax.h"
#include <llvm-c/Core.h>


(executable check - success) clang++
(finished - exit code 1) ['/bin/bash', '-c', '''clang++'' -S -x c++-header -o /dev/null -iquote ''/home/ivan/Complete_Complier'' -I''/home/ivan/Complete_Complier'' -std=c++14 -Wall - < ''/tmp/vK0IHG5/1/codegen.h''']

typedef struct named_value {
    const char* name;
    LLVMValueRef value;
} named_value;

LLVMValueRef codegen(AST* node, LLVMModuleRef module, LLVMBuilderRef builder);
