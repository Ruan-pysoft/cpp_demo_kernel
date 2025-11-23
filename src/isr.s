/* see https://wiki.osdev.org/Interrupt_Service_Routines */
/* also https://wiki.osdev.org/Interrupt_Descriptor_Table */

.section .text

.macro push_msg begin, end
	.set _msg_len, \end - \begin
	push $_msg_len
	push $\begin
.endm

.macro write_msg begin, end
	push_msg \begin, \end
	call blt_write_str
	add $8, %esp
.endm

.macro isr_stub_noerr isr_name
.global \isr_name
\isr_name:
	pushal

	write_msg 1f, 2f
	call blt_newline

	call abort

	popal
	iret
.endm

.macro isr_stub_err isr_name
.global \isr_name
\isr_name:
	pop [error_code]

	pushal

	write_msg 1f, 2f
	call blt_newline

	write_msg error_code_msg, error_code_msg_end

	push [error_code]
	call blt_write_hex
	add $4, %esp

	call blt_newline

	call abort

	popal
	iret
.endm

isr_stub_noerr isr0x00_DE /* divide error */
1: .ascii "division error"
2:

isr_stub_noerr isr0x01_DB /* debug exception */
1: .ascii "debug exception"
2:

isr_stub_noerr isr0x02_NMI /* NMI interrupt */
1: .ascii "NMI interrupt"
2:

isr_stub_noerr isr0x03_BP /* Breakpoint */
1: .ascii "breakpoint"
2:

isr_stub_noerr isr0x04_OF /* overflow */
1: .ascii "overflow fault"
2:

isr_stub_noerr isr0x05_BR /* BOUND range exceeded */
1: .ascii "BOUND range exceeded"
2:

isr_stub_noerr isr0x06_UD /* undefined opcode */
1: .ascii "Hit undefined opcode fault"
2:

isr_stub_noerr isr0x07_NM /* no math coprocessor */
1: .ascii "Hit no math coprocessor fault"
2:

isr_stub_err isr0x08_DF /* double fault */
1: .ascii "Hit double fault"
2:

isr_stub_err isr0x0A_TS /* invalid TSS */
1: .ascii "Hit invalid TSS fault"
2:

isr_stub_err isr0x0B_NP /* segment not present */
1: .ascii "Hit segment not present fault"
2:

isr_stub_err isr0x0C_SS /* stack-segment fault */
1: .ascii "Hit stack-segment fault"
2:

isr_stub_err isr0x0D_GP /* general protection */
1: .ascii "Hit general protection interrupt"
2:

.global isr0x0E_PF /* page fault */
isr0x0E_PF:
	pop [error_code]

	pushal

	write_msg 1f, 2f

	push [error_code]
	call blt_write_hex
	add $4, %esp

	call blt_newline

	call abort

	popal
	iret
1: .ascii "Page fault! Error code: "
2:

isr_stub_noerr isr0x10_MF /* x87 floating point error (math fault) */
1: .ascii "x87 floating point error"
2:

isr_stub_err isr0x11_AC /* alignment check */
1: .ascii "alignment check fault"
2:

isr_stub_noerr isr0x12_MC /* machine check */
1: .ascii "machine check fault"
2:

isr_stub_noerr isr0x13_XM /* SIMD floating point exception */
1: .ascii "SIMD exception"
2:

isr_stub_noerr isr0x14_VE /* virtualisation exception */
1: .ascii "virtualisation exception"
2:

isr_stub_err isr0x15_CP /* control protection exception */
1: .ascii "control exception protection"
2:

/* based on https://wiki.osdev.org/Interrupts_Tutorial */

isr_stub_noerr isr0x0F_INTEL

1: .ascii "intel reserved interrupt"
2:

isr_stub_noerr isr0x16_CPU
isr_stub_noerr isr0x17_CPU
isr_stub_noerr isr0x18_CPU
isr_stub_noerr isr0x19_CPU
isr_stub_noerr isr0x1A_CPU
isr_stub_noerr isr0x1B_CPU
isr_stub_noerr isr0x1C_CPU
isr_stub_noerr isr0x1D_CPU
isr_stub_err isr0x1E_CPU
isr_stub_noerr isr0x1F_CPU

1: .ascii "trigerred CPU exception vector"
2:

.global isr0x20_IRQ /* PIT timer triggers */
isr0x20_IRQ:
	pushal

	call pit_handle_trigger

	mov $0x20, %al
	outb %al, $0x20 /* send EOI to the pic */

	popal
	iret

.global isr0x21_IRQ /* keyboard has data */
isr0x21_IRQ:
	pushal
	cld
	call keyboard_interrupt_handler
	popal
	iret

isr_stub_noerr isr0x22_IRQ
isr_stub_noerr isr0x23_IRQ
isr_stub_noerr isr0x24_IRQ
isr_stub_noerr isr0x25_IRQ
isr_stub_noerr isr0x26_IRQ
isr_stub_noerr isr0x27_IRQ
isr_stub_noerr isr0x28_IRQ
isr_stub_noerr isr0x29_IRQ
isr_stub_noerr isr0x2A_IRQ
isr_stub_noerr isr0x2B_IRQ
isr_stub_noerr isr0x2C_IRQ
isr_stub_noerr isr0x2D_IRQ
isr_stub_noerr isr0x2E_IRQ
isr_stub_noerr isr0x2F_IRQ

1: .ascii "triggered IRQ"
2:

error_code: .word 0
error_code_msg: .ascii "Error code: "
error_code_msg_end:
