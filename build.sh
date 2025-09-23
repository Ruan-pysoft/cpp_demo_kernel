#!/bin/sh

set -x -e

mkdir -p build

./as src/boot.s -o build/boot.o
./cc -c src/kernel.cpp -o build/kernel.o
./link

if grub-file --is-x86-multiboot build/myos.bin; then
	echo multiboot confirmed
else
	echo the file is not multiboot
	exit 1
fi
