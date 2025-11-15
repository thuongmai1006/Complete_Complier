#include "symbol_table.h"
static LLVMContextRef TheContext;
static LLVMModuleRef TheModule;
static LLVMBuilderRef TheBuilder;
static HashMap* NamedValues;
LLVMValueRef codegen_number(AST* node);

static void InitializeModule(){
	TheContext = LLVMContextCreate();
	TheModule = LLVMModuleCreateWithNameInContext("JIT", TheContext);
	TheBuilder = LLVMCreateBuilderInContext(TheContext);
	NamedValues = create_map();
}

LLVMValueRef codegen_var(AST* node){
	LLVMValueRef V = map_get(NamedValues, node->name);
	if (!V) {
		fprintf(stderr, "Unknown variable name");
		return NULL;
	}
	return V;
}

LLVMValueRef codegen_call(AST* node, LLVMModuleRef module, LLVMBuilderRef builder){
	LLVMValueRef callee = LLVMGetNamedFunction(module, node->name);
	if (!callee) {
		fprintf(stderr, "Error: Function not in global module table. %s\n", node->name);
		return NULL;
	}
	size_t no_args = LLVMCountParams(callee);
	if (no_args != node->param_count) {
		fprintf(stderr, "Error: Incorrect number of arguemnts passed\n");
		return NULL;
	}

	LLVMValueRef args[node->param_count];
	for (int i = 0; i < node->param_count; ++i) {
		if (node->stmnts[i]->type == AST_NUM) {
			args[i] = codegen_number(node->stmnts[i]);
		}
		else args[i] = codegen_var(node->stmnts[i]);
		if (!args[i]) return NULL;
	}
	return LLVMBuildCall2(builder, LLVMGetElementType(LLVMTypeOf(callee)),
	                      callee, args, node->param_count, "calltmp");
}

LLVMValueRef codegen_prototype(AST* node) {
    int arg_count = node->param_count;
	LLVMTypeRef *param_types = malloc(sizeof(LLVMTypeRef) * arg_count);
	for (int i = 0; i < arg_count; i++) {
		param_types[i] = LLVMDoubleType();
	}

	LLVMTypeRef return_type = LLVMDoubleType();
    LLVMTypeRef function_type = LLVMFunctionType(return_type, param_types, arg_count, 0);
    LLVMValueRef function = LLVMAddFunction(TheModule, node->name, function_type);

	for (int i = 0; i < arg_count; i++) {
		LLVMValueRef param = LLVMGetParam(function, i);
		LLVMSetValueName(param, node->stmnts[i]->name);
	}

	free(param_types);
	return function;
}

LLVMValueRef codegen_function(AST *node){
    LLVMValueRef f = LLVMGetNamedFunction(TheModule, node->name);
    if (!f) f = codegen_prototype(node);
    if (!f) return NULL;
    return f;
}
LLVMValueRef codegen_number(AST* node){
	if (node == NULL) {
		return NULL;
	}

	switch (node->op.type) {
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
	if (lhs == NULL || rhs == NULL) {
		return NULL;
	}
	// Check that both are of the same type
	if (LLVMTypeOf(lhs) != LLVMTypeOf(rhs)) {
		return NULL;
	}
	// Integer Operations
	if (LLVMTypeOf(lhs) == LLVMInt32Type()) {
		switch(node->op.type) {
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
	// Float/Double Operations
	if (LLVMTypeOf(lhs) == LLVMFloatType() || LLVMTypeOf(lhs) == LLVMDoubleType()) {
		switch(node->op.type) {
		case TOK_PLUS:
			return LLVMBuildFAdd(builder, lhs, rhs, "addtmp");
		case TOK_MINUS:
			return LLVMBuildFSub(builder, lhs, rhs, "subtmp");
		case TOK_MUL:
			return LLVMBuildFMul(builder, lhs, rhs, "multmp");
		case TOK_DIV:
			return LLVMBuildFDiv(builder, lhs, rhs, "divtmp");
		default:
			return NULL;
		}
	}
	return NULL;
}

void codegen(AST* node){
	fprintf(stderr, "\n========== CODEGEN START ==========\n");

	if (node == NULL) {
		fprintf(stderr, "Error: Cannot codegen NULL node\n");
		return;
	}

	InitializeModule();

	// Create a function with 2 parameters
	LLVMTypeRef param_types[] = {LLVMInt32Type(), LLVMInt32Type()};
	LLVMTypeRef func_type = LLVMFunctionType(LLVMInt32Type(), param_types, 2, 0);

	LLVMValueRef function = LLVMAddFunction(TheModule, "calculate", func_type);

	// Create a basic block
	LLVMBasicBlockRef entry = LLVMAppendBasicBlock(function, "entry");
	LLVMPositionBuilderAtEnd(TheBuilder, entry);

	// Use the parameters as operands (can't be constant-folded)
	LLVMValueRef param1 = LLVMGetParam(function, 0);
	LLVMValueRef param2 = LLVMGetParam(function, 1);

	// Build the operation with parameters
	LLVMValueRef result = LLVMBuildAdd(TheBuilder, param1, param2, "addtmp");

	LLVMBuildRet(TheBuilder, result);

	fprintf(stderr, "\n=== Generated IR ===\n");
	LLVMDumpModule(TheModule);
	fprintf(stderr, "========== CODEGEN END ==========\n\n");
}
