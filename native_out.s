	.text
	.file	"JIT"
	.globl	choice
	.p2align	4, 0x90
	.type	choice,@function
choice:
	.cfi_startproc
	movl	%edi, %eax
	movl	$-1, %ecx
	.p2align	4, 0x90
.LBB0_1:
	addl	$1, %ecx
	cmpl	$10, %ecx
	jl	.LBB0_1
	movl	%eax, %ecx
	addl	%esi, %ecx
	je	.LBB0_3
	retq
.LBB0_3:
	movl	%esi, %eax
	retq
.Lfunc_end0:
	.size	choice, .Lfunc_end0-choice
	.cfi_endproc

	.section	".note.GNU-stack","",@progbits
