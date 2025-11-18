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

LLVMValueRef codegen_binary_expr(AST* node){
	if (node == NULL) {
		return NULL;
	}
	if (node->left && node->right) {
		fprintf(stderr, "Left: %d Right: %d\n", node->left->type, node->right->type);
	}
    else if(node->left) {
		fprintf(stderr, "Left: %d\n", node->left->type);
	}
    else if(node->right) {
		fprintf(stderr, "Right: %d\n", node->right->type);
	} else fprintf(stderr, "No children :(\n");
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
			return LLVMBuildAdd(TheBuilder, lhs, rhs, "addtmp");
		case TOK_MINUS:
			return LLVMBuildSub(TheBuilder, lhs, rhs, "subtmp");
		case TOK_MUL:
			return LLVMBuildMul(TheBuilder, lhs, rhs, "multmp");
		case TOK_DIV:
			return LLVMBuildFDiv(TheBuilder, lhs, rhs, "divtmp");
		default:
			return NULL;
		}
	}
	// Float/Double Operations
	if (LLVMTypeOf(lhs) == LLVMFloatType() || LLVMTypeOf(lhs) == LLVMDoubleType()) {
		switch(node->op.type) {
		case TOK_PLUS:
			return LLVMBuildFAdd(TheBuilder, lhs, rhs, "addtmp");
		case TOK_MINUS:
			return LLVMBuildFSub(TheBuilder, lhs, rhs, "subtmp");
		case TOK_MUL:
			return LLVMBuildFMul(TheBuilder, lhs, rhs, "multmp");
		case TOK_DIV:
			return LLVMBuildFDiv(TheBuilder, lhs, rhs, "divtmp");
		default:
			return NULL;
		}
	}
	return NULL;
}

LLVMValueRef codegen_call(AST* node){
	LLVMValueRef callee = LLVMGetNamedFunction(TheModule, node->name);
	if (!callee) {
		fprintf(stderr, "Error: Function not in global module table. %s\n", node->name);
		return NULL;
	}
	size_t no_args = LLVMCountParams(callee);
	if (no_args != node->param_cnt) {
		fprintf(stderr, "Error: Incorrect number of arguemnts passed\n");
		return NULL;
	}

	LLVMValueRef args[node->param_cnt];
	for (int i = 0; i < node->param_cnt; ++i) {
		if (node->params[i]->type == AST_NUM) {
			args[i] = codegen_number(node->params[i]);
		}
		else args[i] = codegen_var(node->params[i]);
		if (!args[i]) return NULL;
	}
	return LLVMBuildCall2(TheBuilder, LLVMGetElementType(LLVMTypeOf(callee)),
	                      callee, args, node->param_cnt, "calltmp");
}

LLVMValueRef codegen_prototype(AST* node) {
	int arg_count = node->param_cnt;
	LLVMTypeRef *param_types = malloc(sizeof(LLVMTypeRef) * arg_count);
	for (int i = 0; i < arg_count; i++) {
		TokenType tok_ty = node->params[i]->op.type;
		if (tok_ty == TOK_INT_VAR) param_types[i] = LLVMInt32Type();
		else if (tok_ty == TOK_FLOAT_VAR) param_types[i] = LLVMFloatType();
		else param_types[i] = NULL;
	}
	TokenType retTy = node->op.type;
	LLVMTypeRef return_type = NULL;
	if (retTy == TOK_INT_VAR) {
		return_type = LLVMInt32Type();
	}
	if (retTy == TOK_FLOAT_VAR) {
		return_type = LLVMFloatType();
	}

	LLVMTypeRef function_type = LLVMFunctionType(return_type, param_types, arg_count, 0);
	LLVMValueRef function = LLVMAddFunction(TheModule, node->name, function_type);

	for (int i = 0; i < arg_count; i++) {
		LLVMValueRef param = LLVMGetParam(function, i);
		LLVMSetValueName(param, node->params[i]->name);
	}

	free(param_types);
	return function;
}

LLVMValueRef codegen_function(AST *node){
	LLVMValueRef f = LLVMGetNamedFunction(TheModule, node->name);
	if (!f) f = codegen_prototype(node);
	if (!f) return NULL;
	LLVMBasicBlockRef bb = LLVMAppendBasicBlockInContext(TheContext, f, "entry");
	LLVMPositionBuilderAtEnd(TheBuilder, bb);
	map_clear(NamedValues);
	size_t arg_count = LLVMCountParams(f);
	for (size_t i = 0; i < arg_count; ++i) {
		LLVMValueRef param = LLVMGetParam(f, i);
		map_put(NamedValues, LLVMGetValueName(param), param);
	}
	LLVMValueRef last_value = NULL;
	for (size_t i = 0; i < node->stmnt_cnt; ++i) {
        int AST_TYPE = node->stmnts[i]->type;
        fprintf(stderr, "Node Type: %d\n", AST_TYPE);
        if (AST_TYPE == AST_BINOP) last_value = codegen_binary_expr(node->stmnts[i]);
        if (AST_TYPE == AST_RETURN) last_value = codegen_binary_expr(node->stmnts[i]->right);
		if (last_value == NULL) return NULL;
	}
	LLVMBuildRet(TheBuilder, last_value);
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

void codegen(AST* node){
	fprintf(stderr, "\n========== CODEGEN START ==========\n");

	if (node == NULL) {
		fprintf(stderr, "Error: Cannot codegen NULL node\n");
		return;
	}

	InitializeModule();
	LLVMValueRef function;
	if (node->type == AST_FUNC) {
		function = codegen_function(node);
	} else{
		// Create a function with 2 parameters
		LLVMTypeRef param_types[] = {LLVMInt32Type(), LLVMInt32Type()};
		LLVMTypeRef func_type = LLVMFunctionType(LLVMInt32Type(), param_types, 2, 0);

		function = LLVMAddFunction(TheModule, "calculate", func_type);

		// Create a basic block
		LLVMBasicBlockRef entry = LLVMAppendBasicBlock(function, "entry");
		LLVMPositionBuilderAtEnd(TheBuilder, entry);

		// Use the parameters as operands (can't be constant-folded)
		LLVMValueRef param1 = LLVMGetParam(function, 0);
		LLVMValueRef param2 = LLVMGetParam(function, 1);

		// Build the operation with parameters
		LLVMValueRef result = LLVMBuildAdd(TheBuilder, param1, param2, "addtmp");

		LLVMBuildRet(TheBuilder, result);
	}
	fprintf(stderr, "\n=== Generated IR ===\n");
	LLVMDumpModule(TheModule);
	fprintf(stderr, "========== CODEGEN END ==========\n\n");
	LLVMDeleteFunction(function);
}
