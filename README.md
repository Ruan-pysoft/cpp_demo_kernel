Based on the [Bare Bones](https://wiki.osdev.org/Bare_Bones) and the [Meaty Skeleton](https://wiki.osdev.org/Meaty_Skeleton) guides on the osdev wiki.

Run `source profile.sh` to add the cross-compilation binaries to your path.

Run `./clean.sh && build.sh` to clean build the kernel.

Run `qemu-system-i386 -s -kernel build/myos.bin` to run the kernel.

Kernel built in C++ for the CMPG121 project at NWU.

I only build the kernel image directly (`myos.bin`) and do not construct a `sysroot/` to make an iso, because I don't wanna deal with grub and I only have like four weeks for this anyways so there's no ways I'm getting anywhere near a userspace, so the whole "OS" is going to be a single ELF binary. That is, the kernel will contain the "userspace", which will just be a couple of demo programs. For this reason, very little thought is given for future proofing at all.

This decision also has other effects: There is no point in setting up a GDT or paging, rather I'll just work with raw addresses and have read/write/execute permission everywhere. Similarly, no thought is given to future multithreading; I've been working on a [c runtime](https://sr.ht/~ruan_p/ministdlib) for a few *months* now and my multithreading support is still half-arsed, and that is *with* operating system support from the get-go!

## TODO/Look into

I might try switching over to [nob.h](https://github.com/nob.h) if managing the builds through a `.sh` file becomes too unwieldy. (I could also do a `Makefile`, and while I *can* do makefiles, and I *do* have other projects I can steal makefiles from, makefiles are weird and icky and I don't like them)
