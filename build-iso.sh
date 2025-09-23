#!/bin/sh

set -x -e

cd build

mkdir -p isodir/boot/grub
cp myos.bin isodir/boot/myos.bin
cp ../grub.cfg isodir/boot/grub/grub.cfg
grub-mkrescue -o myos.iso isodir
