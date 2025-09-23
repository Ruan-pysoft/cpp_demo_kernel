/* from https://wiki.osdev.org/Calling_Global_Constructors */

.section .init
.global _init
.type _init, @function
_init:
	/* I think this is setting up the stack frame? */
	push %ebp
	movl %esp, %ebp
	/* g++ should put crtbegin.o's .init section here */

.section .fini
.global _fini
.type _fini, @function
_fini:
	/* same as in _init, set up the stack frame */
	push %ebp
	movl %esp, %ebp
	/* g++ should put crtbegin.o's .fini section here */
