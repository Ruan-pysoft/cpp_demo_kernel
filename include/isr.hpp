#pragma once

// a list of all the interrupt service routines

#define ISRS \
	X(0x00, DE) \
	X(0x01, DB) \
	X(0x02, NMI) \
	X(0x03, BP) \
	X(0x04, OF) \
	X(0x05, BR) \
	X(0x06, UD) \
	X(0x07, NM) \
	X(0x08, DF) \
	X(0x0A, TS) \
	X(0x0B, NP) \
	X(0x0C, SS) \
	X(0x0D, GP) \
	X(0x0E, PF) \
	X(0x0F, INTEL) \
	X(0x10, MF) \
	X(0x11, AC) \
	X(0x12, MC) \
	X(0x13, XM) \
	X(0x14, VE) \
	X(0x15, CP) \
	X(0x16, CPU) \
	X(0x17, CPU) \
	X(0x18, CPU) \
	X(0x19, CPU) \
	X(0x1A, CPU) \
	X(0x1B, CPU) \
	X(0x1C, CPU) \
	X(0x1D, CPU) \
	X(0x1E, CPU) \
	X(0x1F, CPU)
#define X(isr_num, isr_id) extern "C" void isr ## isr_num ## _ ## isr_id(void);
ISRS
#undef X
