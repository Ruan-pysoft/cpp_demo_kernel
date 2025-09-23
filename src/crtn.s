/* from https://wiki.osdev.org/Calling_Global_Constructors */

.section .init
	/* clean up the stack frame */
	/* g++ should put crtend.o's .init section here */
	popl %ebp
	ret

.section .fini
	/* clean up the stack frame */
	/* g++ should put crtend.o's .fini section here */
	popl %ebp
	ret
