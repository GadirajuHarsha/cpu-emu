	.arch armv8-a
	.text
	.align	2
	.p2align 3,,7
	.global	foo
	.type	foo, %function
foo:
    add x1, x30, #0
    add x2, x2, #123
    add x3, x3, #234
    add x4, x4, #345
    ret x1
    nop
    nop
    nop
	.size	foo, .-foo
    .align	2
	.p2align 3,,7
	.global	start
	.type	start, %function
start:
    // Simple tests for bl and ret instructions.
    adrp x0, start
    sub sp, sp, #16
    nop
    nop
    nop
    stur x30, [sp]
    nop
    nop
    nop
    bl foo
    nop
    nop
    nop
    add x0, x0, :lo12:start
    ldur x30, [sp]
    add sp, sp, #16
    add x0, x0, #40
    nop
    nop
    nop
    ret x0
    nop
    nop
    nop
	ret
	.size	start, .-start
	.ident	"GCC: (Ubuntu/Linaro 7.5.0-3ubuntu1~18.04) 7.5.0"
	.section	.note.GNU-stack,"",@progbits
