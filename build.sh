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
CFLAGS="$CFLAGS -I./include"

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

$AS $SRCDIR/boot.s -o build/boot.o
$CC $CFLAGS -c $SRCDIR/kernel.cpp -o $BUILDDIR/kernel.o
$CC $CFLAGS -c $SRCDIR/vga.cpp -o $BUILDDIR/vga.o
$CC $CFLAGS -c $SRCDIR/string.cpp -o $BUILDDIR/string.o
$AS $SRCDIR/ioport.s -o $BUILDDIR/ioport.o
$AS $SRCDIR/crti.s -o $BUILDDIR/crti.o
$AS $SRCDIR/crtn.s -o $BUILDDIR/crtn.o

# link the kernel

$LD -T $SRCDIR/linker.ld $LDFLAGS -o "$BUILDDIR/$KERNELNAME.bin" \
	$BUILDDIR/crti.o $CRTBEGIN_OBJ \
	$BUILDDIR/boot.o $BUILDDIR/kernel.o $BUILDDIR/vga.o $BUILDDIR/string.o \
	$BUILDDIR/ioport.o \
	$CRTEND_OBJ $BUILDDIR/crtn.o \
	$LD_LFLAGS

if grub-file --is-x86-multiboot build/myos.bin; then
	echo multiboot confirmed
else
	echo the file is not multiboot
	exit 1
fi
