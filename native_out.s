	.text
	.file	"JIT"
	.globl	calculate
	.p2align	4, 0x90
	.type	calculate,@function
calculate:
	.cfi_startproc
	leal	(%rdi,%rsi), %eax
	retq
.Lfunc_end0:
	.size	calculate, .Lfunc_end0-calculate
	.cfi_endproc

	.section	".note.GNU-stack","",@progbits
