A quick document saying where everything because I haven't touched this code in a long time.

Linker script: `src/linker.ld`

Entry point, setup, teardown: `src/boot.s`

Global constructors/destructors: `src/crti.s` + `src/crtn.s`

Main code: `src/kernel.c`

VGA text-mode interface: `src/vga.cpp` + `include/vga.hpp`

Debug printing (minimal dependencies & state): `src/blit.cpp`, `include/blit.hpp`

IO port utilities: `src/ioport.s` + `include/ioport.hpp`

Code to reload segments after loading GDT: `reload_segments.nasm` + `include/reload_segments.hpp`  
I'm pretty sure I couldn't figure out how to do this with GNU's `as` or gcc's inline assembly, which is why it's using NASM. I think I should probably port all the other assembly files to NASM and drop the GNU `as` dependency.

ISRs: `src/isr.s` + `include/isr.hpp`

PIC initialisation + basic utilities: `src/pic.cpp` + `include/pic.hpp`

PS2 keyboard interface + initialisation: `src/ps2.cpp` + `include/ps2.hpp`
