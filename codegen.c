#include "symbol_table.h"
static LLVMContextRef TheContext;
static LLVMModuleRef TheModule;
static LLVMBuilderRef TheBuilder;
static HashMap* NamedValues;
LLVMValueRef codegen(AST* node);
void codegen_run();
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
		fprintf(stderr, "Unknown variable name: %s\n", node->name);
		return NULL;
	}
	return V;
}
LLVMValueRef codegen_if(AST* node){
	LLVMValueRef cond = codegen(node->cond);
    cond = LLVMBuildICmp(TheBuilder, LLVMIntSGT, cond, LLVMConstInt(LLVMInt32Type(), 0, 0), "ifcond");
    LLVMValueRef f = LLVMGetBasicBlockParent(LLVMGetInsertBlock(TheBuilder));
	if (!cond) return NULL;
    if (!f) return NULL;
	LLVMBasicBlockRef thenBB = LLVMAppendBasicBlockInContext(TheContext, f, "then");
	LLVMBasicBlockRef elseBB = LLVMCreateBasicBlockInContext(TheContext, "else");
	LLVMBasicBlockRef mergeBB = LLVMCreateBasicBlockInContext(TheContext, "ifcont");
	LLVMValueRef condbr = LLVMBuildCondBr(TheBuilder, cond, thenBB, elseBB);

    LLVMPositionBuilderAtEnd(TheBuilder, thenBB);
    LLVMValueRef thenV = codegen(node->left); // then statements stored in AST_IF's left node 
    LLVMValueRef then_br = LLVMBuildBr(TheBuilder, mergeBB);
    thenBB = LLVMGetInsertBlock(TheBuilder);

    LLVMAppendExistingBasicBlock(f, elseBB);
    LLVMPositionBuilderAtEnd(TheBuilder, elseBB);
    LLVMValueRef elseV = codegen(node->right->left);
    LLVMValueRef else_br = LLVMBuildBr(TheBuilder, mergeBB);
    elseBB = LLVMGetInsertBlock(TheBuilder);

    LLVMAppendExistingBasicBlock(f, mergeBB);
    LLVMPositionBuilderAtEnd(TheBuilder, mergeBB);
    LLVMValueRef PN = LLVMBuildPhi(TheBuilder, LLVMInt32Type(),"iftmp");
    LLVMValueRef incomingValues[] = {thenV, elseV};
    LLVMBasicBlockRef incomingBlocks[] = {thenBB, elseBB};
    LLVMAddIncoming(PN,incomingValues, incomingBlocks, 2); 
    return PN;
}

LLVMValueRef codegen_binary_expr(AST* node){
	if (node == NULL) {
		return NULL;
	}
	LLVMValueRef lhs = codegen(node->left);
	LLVMValueRef rhs = codegen(node->right);
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
		args[i] = codegen(node->params[i]);
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
	fprintf(stderr, "Function name: %s\n", node->name);
	LLVMValueRef function = LLVMAddFunction(TheModule, node->name, function_type);

	for (int i = 0; i < arg_count; i++) {
		LLVMValueRef param = LLVMGetParam(function, i);
		const char* param_name = node->params[i]->left->name;
		fprintf(stderr, "params[%d] = %s\n", i, param_name);
		LLVMSetValueName(param,param_name);
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
		const char* name = LLVMGetValueName(param);
		map_put(NamedValues,name, param);
		fprintf(stderr, "Placing %s into the hashmap\n", name);
	}
	LLVMValueRef last_value = NULL;
	for (size_t i = 0; i < node->stmnt_cnt; ++i) {
		int AST_TYPE = node->stmnts[i]->type;
		fprintf(stderr, "Node Type: %d\n", AST_TYPE);
		last_value = codegen(node->stmnts[i]);
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

LLVMValueRef codegen(AST* node){
	switch(node->type) {
	case AST_NUM: return codegen_number(node);
	case AST_ID: return codegen_var(node);
	case AST_BINOP: return codegen_binary_expr(node);
	case AST_RETURN: return codegen(node->right);
    case AST_IF: return codegen_if(node);
	default: {
		fprintf(stderr, "We defaulting here\n");
		return NULL;
	}
	}
}
void codegen_run(AST* node){
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
    char *error = NULL;
    if (LLVMPrintModuleToFile(TheModule, "output.ll", &error)){
        fprintf(stderr, "Error writing IR: %s\n", error);
        LLVMDisposeMessage(error);
    }
	LLVMDumpModule(TheModule);
	fprintf(stderr, "========== CODEGEN END ==========\n\n");
}
