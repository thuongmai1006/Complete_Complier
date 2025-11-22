	.text
	.abicalls
	.option	pic0
	.section	.mdebug.abi32,"",@progbits
	.nan	legacy
	.text
	.file	"JIT"
	.globl	choice
	.p2align	2
	.type	choice,@function
	.set	nomicromips
	.set	nomips16
	.ent	choice
choice:
	.cfi_startproc
	.frame	$sp,0,$ra
	.mask 	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	.set	noat
	addiu	$2, $zero, -1
$BB0_1:
	addiu	$2, $2, 1
	slti	$1, $2, 10
	bnez	$1, $BB0_1
	nop
	addu	$1, $4, $5
	beqz	$1, $BB0_4
	nop
	jr	$ra
	move	$2, $4
$BB0_4:
	move	$4, $5
	jr	$ra
	move	$2, $4
	.set	at
	.set	macro
	.set	reorder
	.end	choice
$func_end0:
	.size	choice, ($func_end0)-choice
	.cfi_endproc

	.section	".note.GNU-stack","",@progbits
	.text
