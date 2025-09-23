Based on the [Bare Bones](https://wiki.osdev.org/Bare_Bones) and the [Meaty Skeleton](https://wiki.osdev.org/Meaty_Skeleton) guides on the osdev wiki.

Run `source profile.sh` to add the cross-compilation binaries to your path.

Kernel built in C++ for the CMPG121 project at NWU.

I only build the kernel image directly (`myos.bin`) and do not construct a `sysroot/` to make an iso, because I don't wanna deal with grub and I only have like four weeks for this anyways so there's no ways I'm getting anywhere near a userspace, so the whole "OS" is going to be a single ELF binary. That is, the kernel will contain the "userspace", which will just be a couple of demo programs. For this reason, very little thought is given for future proofing at all.

This decision also has other effects: There is no point in setting up a GDT or paging, rather I'll just work with raw addresses and have read/write/execute permission everywhere. Similarly, no thought is given to future multithreading; I've been working on a [c runtime](https://sr.ht/~ruan_p/ministdlib) for a few *months* now and my multithreading support is still half-arsed, and that is *with* operating system support from the get-go!
