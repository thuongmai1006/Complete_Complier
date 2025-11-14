// main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Transforms/PassManagerBuilder.h>

//---------------------
// Build IR: int add(int a, int b) { return a + b; }
//---------------------
static LLVMModuleRef build_module(LLVMContextRef ctx) {
    LLVMModuleRef mod = LLVMModuleCreateWithNameInContext("minicc_module", ctx);
    LLVMBuilderRef b = LLVMCreateBuilderInContext(ctx);

    LLVMTypeRef i32 = LLVMInt32TypeInContext(ctx);
    LLVMTypeRef params[2] = { i32, i32 };
    LLVMTypeRef fnty = LLVMFunctionType(i32, params, 2, 0);
    LLVMValueRef fn = LLVMAddFunction(mod, "add", fnty);

    LLVMValueRef a = LLVMGetParam(fn, 0);
    LLVMSetValueName2(a, "a", 1);
    LLVMValueRef bb = LLVMGetParam(fn, 1);
    LLVMSetValueName2(bb, "b", 1);

    LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(ctx, fn, "entry");
    LLVMPositionBuilderAtEnd(b, entry);
    LLVMValueRef sum = LLVMBuildAdd(b, a, bb, "sum");
    LLVMBuildRet(b, sum);

    // Verify
    char *err = NULL;
    if (LLVMVerifyModule(mod, LLVMAbortProcessAction, &err)) {
        fprintf(stderr, "verify failed: %s\n", err);
        LLVMDisposeMessage(err);
        exit(1);
    }
    LLVMDisposeBuilder(b);
    return mod;
}

//---------------------
// Option A: JIT (MCJIT via C API)
//---------------------
static void jit_and_run(LLVMModuleRef mod) {
    // MCJIT needs these
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

    char *err = NULL;
    // Or: LLVMCreateJITCompilerForModule(&ee, mod, optlevel, &err)
    LLVMExecutionEngineRef ee = NULL;
    if (LLVMCreateJITCompilerForModule(&ee, mod, /*optlevel=*/2, &err)) {
        fprintf(stderr, "JIT create error: %s\n", err);
        LLVMDisposeMessage(err);
        exit(1);
    }

    // Get pointer to compiled function
    uint64_t addr = LLVMGetFunctionAddress(ee, "add");
    typedef int (*add_fn_t)(int, int);
    add_fn_t add_fn = (add_fn_t)(uintptr_t)addr;

    int got = add_fn(20, 22);
    printf("JIT add(20,22) = %d\n", got);

    LLVMDisposeExecutionEngine(ee); // also disposes the module it owns
}

//---------------------
// Option B: Emit object file (out.o)
//---------------------
static void emit_object_file(LLVMContextRef ctx) {
    // Build a fresh module (the JIT took ownership of the prior one)
    LLVMModuleRef mod = build_module(ctx);

    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();

    char *triple = LLVMGetDefaultTargetTriple();

    LLVMTargetRef target;
    char *err = NULL;
    if (LLVMGetTargetFromTriple(triple, &target, &err)) {
        fprintf(stderr, "No target for triple %s: %s\n", triple, err);
        LLVMDisposeMessage(err);
        exit(1);
    }

    LLVMTargetMachineRef tm = LLVMCreateTargetMachine(
        target,
        triple,
        "generic",                // CPU
        "",                       // features
        LLVMCodeGenLevelDefault,
        LLVMRelocDefault,
        LLVMCodeModelDefault
    );

    // Set DL + triple on module
    LLVMSetTarget(mod, triple);
    LLVMTargetDataRef dl = LLVMCreateTargetDataLayout(tm);
    LLVMSetModuleDataLayout(mod, dl);

    // (Optional) basic opt pipeline
    LLVMPassManagerRef pm = LLVMCreatePassManager();
    LLVMPassManagerBuilderRef pmb = LLVMPassManagerBuilderCreate();
    LLVMPassManagerBuilderSetOptLevel(pmb, 2);
    LLVMPassManagerBuilderPopulateModulePassManager(pmb, pm);
    LLVMRunPassManager(pm, mod);
    LLVMPassManagerBuilderDispose(pmb);
    LLVMDisposePassManager(pm);

    // Emit .o
    char *outfile = "out.o";
    if (LLVMTargetMachineEmitToFile(tm, mod, outfile, LLVMObjectFile, &err)) {
        fprintf(stderr, "Emit error: %s\n", err);
        LLVMDisposeMessage(err);
        exit(1);
    }
    printf("Wrote %s\n", outfile);

    // Cleanup
    LLVMDisposeTargetMachine(tm);
    LLVMDisposeTargetData(dl);
    LLVMDisposeMessage(triple);
    LLVMDisposeModule(mod);
}

int main(void) {
    LLVMContextRef ctx = LLVMContextCreate();

    // Build, print IR
    LLVMModuleRef mod = build_module(ctx);
    char *ir = LLVMPrintModuleToString(mod);
    puts(ir);
    LLVMDisposeMessage(ir);

    // JIT & call
    jit_and_run(mod); // takes ownership of 'mod'

    // Emit object file
    emit_object_file(ctx);

    LLVMContextDispose(ctx);
    return 0;
}
