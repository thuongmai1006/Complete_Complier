	.text
	.abicalls
	.option	pic0
	.section	.mdebug.abi32,"",@progbits
	.nan	legacy
	.text
	.file	"JIT"
	.globl	calculate
	.p2align	2
	.type	calculate,@function
	.set	nomicromips
	.set	nomips16
	.ent	calculate
calculate:
	.cfi_startproc
	.frame	$sp,0,$ra
	.mask 	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	.set	noat
	jr	$ra
	addu	$2, $4, $5
	.set	at
	.set	macro
	.set	reorder
	.end	calculate
$func_end0:
	.size	calculate, ($func_end0)-calculate
	.cfi_endproc

	.section	".note.GNU-stack","",@progbits
	.text
