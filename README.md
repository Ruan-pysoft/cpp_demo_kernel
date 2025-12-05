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

## Licensing

NB: Note that the copyright of the Bible quotations found in various places in the project as sample text belongs to _die Bybelgenootskap van Suid-Afrika_ (the Bible society of South Africa) and is taken from the Afrikaans 2020 translation. I in no way, shape, or form claim copyright over it, nor does the licensing of my code apply to it. I believe my use of these quotations falls in line with the permissions granted on the page containing the copyright info for the text in my copy of the Bible.

This kernel is set free under the [Unlicense](https://unlicense.org/).
This means that it is released into the public domain
for all to use as they see fit.
Use it in a way I'll approve of,
use it in a way I won't,
doesn't make much of a difference to me.

I only ask (as a request in the name of common courtesy,
**not** as a legal requirement of any sort)
that you do not claim this work as your own
but credit me as appropriate.

The full terms are as follows:

```
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <https://unlicense.org/>
```
