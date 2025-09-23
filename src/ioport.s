.section .text

.global outb
.type outb, @function
outb:
	/* output a byte to the specified port */
	/* stack:
	 *      [ byte ]        1 byte
	 *      [ port ]        4 bytes
	 * top: [ return addr ] 4 bytes
	 */

	movw 4(%esp), %dx /* port */
	movb 8(%esp), %al /* byte */

	outb %al, %dx

	ret
.global outw
.type outw, @function
outw:
	/* output a word (16 bits) to the specified port */
	/* stack:
	 *      [ word ]        2 bytes
	 *      [ port ]        4 bytes
	 * top: [ return addr ] 4 bytes
	 */

	movw 4(%esp), %dx /* port */
	movw 8(%esp), %ax /* word */

	out %ax, %dx

	ret
.global outdw
.type outdw, @function
outdw:
	/* output a double word (32 bits) to the specified port */
	/* stack:
	 *      [ double word ] 4 bytes
	 *      [ port ]        4 bytes
	 * top: [ return addr ] 4 bytes
	 */

	movw 4(%esp), %dx  /* port */
	movl 8(%esp), %eax /* double word */

	outl %eax, %dx

	ret

.global inb
.type inb, @function
inb:
	/* input a byte from the specified port */
	/* stack:
	 *      [ port ]        4 bytes
	 * top: [ return addr ] 4 bytes
	 */

	movw 4(%esp), %dx /* port */

	inb %dx, %al

	ret
.global inw
.type inw, @function
inw:
	/* input a word (16 bits) from the specified port */
	/* stack:
	 *      [ port ]        4 bytes
	 * top: [ return addr ] 4 bytes
	 */

	movw 4(%esp), %dx /* port */

	in %dx, %ax

	ret
.global indw
.type indw, @function
indw:
	/* input a double word (32 bits) from the specified port */
	/* stack:
	 *      [ port ]        4 bytes
	 * top: [ return addr ] 4 bytes
	 */

	movw 4(%esp), %dx /* port */

	inl %dx, %eax

	ret
