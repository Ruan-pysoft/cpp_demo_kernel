Based on the [Bare Bones](https://wiki.osdev.org/Bare_Bones) and the [Meaty Skeleton](https://wiki.osdev.org/Meaty_Skeleton) guides on the osdev wiki.

Run `source profile.sh` to add the cross-compilation binaries to your path.

Run `./clean.sh && build.sh` to clean build the kernel.

Run `qemu-system-i386 -s -kernel build/myos.bin` to run the kernel.

Kernel built in C++. Originally for the CMPG121 project at NWU, but then I got stuck trying to implement the PS/2 interface for two days and switched to doing [a platformer with Raylib](https://github.com/Ruan-pysoft/platformer) instead.

I only build the kernel image directly (`myos.bin`) and do not create an iso with a bootloader or filesystem, as I don't want to deal with grub, and I'm never going to support a userspace – I'm instead planning on compiling the whole "OS" into a single ELF binary. That is, the whole "userspace", consisting of a few demo programs, will be compiled into the kernel. There is also little thought given to future proofing, as I plan on keeping the kernel very small in scope.

This decision also has other effects: There is no point in setting up paging, rather I'll just work with a flat memory model and have read/write/execute permission everywhere. Similarly, no thought is given to future multithreading; I've been working on a [c runtime](https://sr.ht/~ruan_p/ministdlib) for a few *months* now and my multithreading support is still half-arsed, and that is *with* operating system support from the get-go!

## TODO/Look into

I might try switching over to [nob.h](https://github.com/nob.h) if managing the builds through a `.sh` file becomes too unwieldy. (I could also do a `Makefile`, and while I *can* do makefiles, and I *do* have other projects I can steal makefiles from, makefiles are weird and icky and I don't like them)

(By now I've used nob.h for a few different projects, including [getting ps/2 support working in a kernel](https://github.com/Ruan-pysoft/ps2keyboard_demo), and I absolutely love it. I'll definitely switch over at some point, the shell script is butt-ugly)
