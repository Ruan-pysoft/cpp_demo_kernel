A quick document saying where everything because I haven't touched this code in a long time.

Linker script: `src/linker.ld`

Entry point, setup, teardown: `src/boot.s`

Global constructors/destructors: `src/crti.s` + `src/crtn.s`

GDT initialisation and loading: `src/gdt.cpp` + `include/gdt.hpp`  
Honestly, compare this to a [GDT implemented in assembly](https://github.com/Ruan-pysoft/ps2keyboard_demo/blob/master/src/gdt.s), and I think I might just rewrite it in assembly at some point, because the C++ looks _ugly_.

IDT initialisation and loading: `src/idt.cpp` + `include/idt.hpp`  
See my comment for `src/gdt.cpp`; [this](https://github.com/Ruan-pysoft/ps2keyboard_demo/blob/master/src/idt.s) just looks so much better

Main code: `src/kernel.c`

VGA text-mode interface: `src/vga.cpp` + `include/vga.hpp`

Debug printing (minimal dependencies & state): `src/blit.cpp`, `include/blit.hpp`

IO port utilities: `src/ioport.s` + `include/ioport.hpp`

Code to reload segments after loading GDT: `reload_segments.nasm` + `include/reload_segments.hpp`  
I'm pretty sure I couldn't figure out how to do this with GNU's `as` or gcc's inline assembly, which is why it's using NASM. I think I should probably port all the other assembly files to NASM and drop the GNU `as` dependency.

ISRs: `src/isr.s` + `include/isr.hpp`

PIC initialisation + basic utilities: `src/pic.cpp` + `include/pic.hpp`

PS2 keyboard interface + initialisation: `src/ps2.cpp` + `include/ps2.hpp`

"Standard library" implementation: `src/libk/` + `include/libk/`

Application programming support libraries: `src/libk/sdk/` + `include/libk/sdk/`

Applications: `src/apps/`

C elements of the standard library is implemented such that each stdlib function has its own file under a folder corresponding to its header.

Currently implemented:
 - `cppsupport.hpp`: support for C++ (new, delete, atexit, etc)
 - `stdio.h`: io functions (higher-level wrapper over the VGA driver)
 - `stdlib.h`: memory allocation + freeing, as well as an abort function
 - `string.h`: some string handling/memory manipulation functions
 - `sys/cdefs.h`: tbh I have no idea

The following application support libraries currently exist:
 - `eventloop.hpp`: Support for three different types of event loops. An event loop object automatically handles keyboard input while sleeping for the next frame, since there is no underlying operating system to do so.
 - `random.hpp`: Defines a random number generation API and defines a random number generator. Possibly to be expanded in the future.
 - `terminal.hpp`: An API to change the terminal's colours and to automatically switch back at the end of the code block via RAII.

The following applications are currently implemented:
 - `main_menu.cpp`: The main menu the user is greeted with when launching the kernel, works as an app launcher of sorts to run the other applications. Some applications are hidden in an "advanced" menu, accessed by pressing the colon (`:`) key.
 - `snake.cpp`: A simple snake game.
 - `forth.cpp`: An implementation of a [forth](https://en.wikipedia.org/wiki/Forth_(programming_language\)) language.
 - `character_map.cpp`: A program which displays all the characters supported by the VGA textmode driver, and allows selecting a character to see its character code.
 - `queued_demo.cpp`: A short proof-of-concept/reference application using a QueuedEventLoop.
 - `callback_demo.cpp`: A short proof-of-concept/reference application using a CallbackEventLoop.
 - `ignore_demo.cpp`: A short proof-of-concept/reference application using a IgnoreEventLoop.
