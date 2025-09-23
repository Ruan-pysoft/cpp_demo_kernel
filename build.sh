#!/bin/sh

set -x -e

# set up some variables

SRCDIR="src/"
BUILDDIR="build/"
KERNELNAME="myos"

CC="$HOME/opt/cross/bin/i686-elf-g++"
CFLAGS="-ffreestanding -O2 -g"
CFLAGS="$CFLAGS -Wall -Wextra -pedantic"
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

$CC $CFLAGS -c $SRCDIR/kernel.cpp -o $BUILDDIR/kernel.o
OBJS="$OBJS $BUILDDIR/kernel.o"
$CC $CFLAGS -c $SRCDIR/vga.cpp -o $BUILDDIR/vga.o
OBJS="$OBJS $BUILDDIR/vga.o"
$AS $SRCDIR/ioport.s -o $BUILDDIR/ioport.o
OBJS="$OBJS $BUILDDIR/ioport.o"

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

$CC $CFLAGS -c $SRCDIR/libk/stdlib/abort.cpp -o $BUILDDIR/libk-stdlib-abort.o
LIBK_OBJS="$LIBK_OBJS $BUILDDIR/libk-stdlib-abort.o"

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
