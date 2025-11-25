#!/bin/sh

set -x -e

# set up some variables

SRCDIR="src/"
BUILDDIR="build/"
KERNELNAME="myos"

CC="$HOME/opt/cross/bin/i686-elf-g++"
CFLAGS="-ffreestanding -O2 -g"
CFLAGS="$CFLAGS -Wall -Wextra"
CFLAGS="$CFLAGS -fno-exceptions -fno-rtti"
CFLAGS="$CFLAGS -I./include -isystem ./include/libk"

AS="$HOME/opt/cross/bin/i686-elf-as"

LD="$HOME/opt/cross/bin/i686-elf-gcc"
LDFLAGS="-O2"
LDFLAGS="$LDFLAGS -nostdlib"
LD_LFLAGS="-lgcc"

CRTBEGIN_OBJ=$($CC -print-file-name=crtbegin.o)
CRTEND_OBJ=$($CC -print-file-name=crtend.o)

# create the build directory

mkdir -p $BUILDDIR

# compile the object files for the kernel

OBJS=""

$AS $SRCDIR/boot.s -o $BUILDDIR/boot.o
OBJS="$OBJS $BUILDDIR/boot.o"
$AS $SRCDIR/isr.s -o $BUILDDIR/isr.o
OBJS="$OBJS $BUILDDIR/isr.o"
nasm -f elf32 $SRCDIR/reload_segments.nasm -o $BUILDDIR/reload_segments.o
OBJS="$OBJS $BUILDDIR/reload_segments.o"

$CC $CFLAGS -c $SRCDIR/kernel.cpp -o $BUILDDIR/kernel.o
OBJS="$OBJS $BUILDDIR/kernel.o"
$CC $CFLAGS -c $SRCDIR/vga.cpp -o $BUILDDIR/vga.o
OBJS="$OBJS $BUILDDIR/vga.o"
$AS $SRCDIR/ioport.s -o $BUILDDIR/ioport.o
OBJS="$OBJS $BUILDDIR/ioport.o"
$CC $CFLAGS -c $SRCDIR/blit.cpp -o $BUILDDIR/blit.o
OBJS="$OBJS $BUILDDIR/blit.o"
$CC $CFLAGS -c $SRCDIR/pic.cpp -o $BUILDDIR/pic.o
OBJS="$OBJS $BUILDDIR/pic.o"
$CC $CFLAGS -c $SRCDIR/pit.cpp -o $BUILDDIR/pit.o
OBJS="$OBJS $BUILDDIR/pit.o"
$CC $CFLAGS -c $SRCDIR/ps2.cpp -o $BUILDDIR/ps2.o
OBJS="$OBJS $BUILDDIR/ps2.o"
$CC $CFLAGS -c $SRCDIR/gdt.cpp -o $BUILDDIR/gdt.o
OBJS="$OBJS $BUILDDIR/gdt.o"
$CC $CFLAGS -c $SRCDIR/idt.cpp -o $BUILDDIR/idt.o
OBJS="$OBJS $BUILDDIR/idt.o"
$CC $CFLAGS -c $SRCDIR/eventloop.cpp -o $BUILDDIR/eventloop.o
OBJS="$OBJS $BUILDDIR/eventloop.o"

mkdir -p $BUILDDIR/apps

$CC $CFLAGS -c $SRCDIR/apps/queued_demo.cpp -o $BUILDDIR/apps/queued_demo.o
OBJS="$OBJS $BUILDDIR/apps/queued_demo.o"
$CC $CFLAGS -c $SRCDIR/apps/callback_demo.cpp -o $BUILDDIR/apps/callback_demo.o
OBJS="$OBJS $BUILDDIR/apps/callback_demo.o"
$CC $CFLAGS -c $SRCDIR/apps/ignore_demo.cpp -o $BUILDDIR/apps/ignore_demo.o
OBJS="$OBJS $BUILDDIR/apps/ignore_demo.o"
$CC $CFLAGS -c $SRCDIR/apps/main_menu.cpp -o $BUILDDIR/apps/main_menu.o
OBJS="$OBJS $BUILDDIR/apps/main_menu.o"
$CC $CFLAGS -c $SRCDIR/apps/snake.cpp -o $BUILDDIR/apps/snake.o
OBJS="$OBJS $BUILDDIR/apps/snake.o"
$CC $CFLAGS -c $SRCDIR/apps/character_map.cpp -o $BUILDDIR/apps/character_map.o
OBJS="$OBJS $BUILDDIR/apps/character_map.o"

LIBK_OBJS=""

# For adding objects (vim, replace stdio with relevant header):
# '<,'>s/\(.*\)\.cpp/$CC $CFLAGS -c $SRCDIR\/libk\/stdio\/\1.cpp -o $BUILDDIR\/libk-stdio-\1.o\rLIBK_OBJS="$LIBK_OBJS $BUILDDIR\/libk-stdio-\1.o"

$CC $CFLAGS -c $SRCDIR/libk/string/memcmp.cpp -o $BUILDDIR/libk-string-memcmp.o
LIBK_OBJS="$LIBK_OBJS $BUILDDIR/libk-string-memcmp.o"
$CC $CFLAGS -c $SRCDIR/libk/string/memcpy.cpp -o $BUILDDIR/libk-string-memcpy.o
LIBK_OBJS="$LIBK_OBJS $BUILDDIR/libk-string-memcpy.o"
$CC $CFLAGS -c $SRCDIR/libk/string/memmove.cpp -o $BUILDDIR/libk-string-memmove.o
LIBK_OBJS="$LIBK_OBJS $BUILDDIR/libk-string-memmove.o"
$CC $CFLAGS -c $SRCDIR/libk/string/memset.cpp -o $BUILDDIR/libk-string-memset.o
LIBK_OBJS="$LIBK_OBJS $BUILDDIR/libk-string-memset.o"
$CC $CFLAGS -c $SRCDIR/libk/string/strlen.cpp -o $BUILDDIR/libk-string-strlen.o
LIBK_OBJS="$LIBK_OBJS $BUILDDIR/libk-string-strlen.o"

$CC $CFLAGS -c $SRCDIR/libk/stdio/printf.cpp -o $BUILDDIR/libk-stdio-printf.o
LIBK_OBJS="$LIBK_OBJS $BUILDDIR/libk-stdio-printf.o"
$CC $CFLAGS -c $SRCDIR/libk/stdio/putchar.cpp -o $BUILDDIR/libk-stdio-putchar.o
LIBK_OBJS="$LIBK_OBJS $BUILDDIR/libk-stdio-putchar.o"
$CC $CFLAGS -c $SRCDIR/libk/stdio/puts.cpp -o $BUILDDIR/libk-stdio-puts.o
LIBK_OBJS="$LIBK_OBJS $BUILDDIR/libk-stdio-puts.o"

$CC $CFLAGS -c $SRCDIR/libk/stdlib/_mm_internals.cpp -o $BUILDDIR/libk-stdlib-_mm_internals.o
LIBK_OBJS="$LIBK_OBJS $BUILDDIR/libk-stdlib-_mm_internals.o"
$CC $CFLAGS -c $SRCDIR/libk/stdlib/abort.cpp -o $BUILDDIR/libk-stdlib-abort.o
LIBK_OBJS="$LIBK_OBJS $BUILDDIR/libk-stdlib-abort.o"
$CC $CFLAGS -c $SRCDIR/libk/stdlib/calloc.cpp -o $BUILDDIR/libk-stdlib-calloc.o
LIBK_OBJS="$LIBK_OBJS $BUILDDIR/libk-stdlib-calloc.o"
$CC $CFLAGS -c $SRCDIR/libk/stdlib/free.cpp -o $BUILDDIR/libk-stdlib-free.o
LIBK_OBJS="$LIBK_OBJS $BUILDDIR/libk-stdlib-free.o"
$CC $CFLAGS -c $SRCDIR/libk/stdlib/malloc.cpp -o $BUILDDIR/libk-stdlib-malloc.o
LIBK_OBJS="$LIBK_OBJS $BUILDDIR/libk-stdlib-malloc.o"
$CC $CFLAGS -c $SRCDIR/libk/stdlib/reallocarray.cpp -o $BUILDDIR/libk-stdlib-reallocarray.o
LIBK_OBJS="$LIBK_OBJS $BUILDDIR/libk-stdlib-reallocarray.o"
$CC $CFLAGS -c $SRCDIR/libk/stdlib/realloc.cpp -o $BUILDDIR/libk-stdlib-realloc.o
LIBK_OBJS="$LIBK_OBJS $BUILDDIR/libk-stdlib-realloc.o"

$CC $CFLAGS -c $SRCDIR/libk/cppsupport.cpp -o $BUILDDIR/libk-cppsupport.o
LIBK_OBJS="$LIBK_OBJS $BUILDDIR/libk-cppsupport.o"

$AS $SRCDIR/crti.s -o $BUILDDIR/crti.o
$AS $SRCDIR/crtn.s -o $BUILDDIR/crtn.o

# link the kernel

$LD -T $SRCDIR/linker.ld $LDFLAGS -o "$BUILDDIR/$KERNELNAME.bin" \
	$BUILDDIR/crti.o $CRTBEGIN_OBJ \
	$OBJS $LIBK_OBJS \
	$CRTEND_OBJ $BUILDDIR/crtn.o \
	$LD_LFLAGS

if grub-file --is-x86-multiboot build/myos.bin; then
	echo multiboot confirmed
else
	echo the file is not multiboot
	exit 1
fi
