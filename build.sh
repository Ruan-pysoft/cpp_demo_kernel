#!/bin/sh

set -x -e

mkdir -p build

./as src/boot.s -o build/boot.o
./cc -c src/kernel.cpp -o build/kernel.o
./cc -c src/vga.cpp -o build/vga.o
./cc -c src/string.cpp -o build/string.o
./as src/ioport.s -o build/ioport.o
./link build/boot.o build/kernel.o build/vga.o build/string.o build/ioport.o

if grub-file --is-x86-multiboot build/myos.bin; then
	echo multiboot confirmed
else
	echo the file is not multiboot
	exit 1
fi
