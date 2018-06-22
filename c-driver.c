#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "dat.h"
#include "fns.h"
#include "ds10.h"

uint8_t *basemem;

uint32_t tmp32;
uint16_t tmp16;
uint8_t tmp8;

//#define vaddr(x,a,b) ((x) < 0x02800000 ? &basemem[((x) - 0x01ff8000) & 0x007fffff]: fault_handler(x,a))
uint32_t *MEM32(uint32_t addr) { return vaddr(addr, 4, 0); }
uint16_t *MEM16(uint32_t addr) { return vaddr(addr, 2, 0); }
uint8_t *MEM8(uint32_t addr) { return vaddr(addr, 1, 0); }

void
check(
		uint32_t addr,
		uint32_t R0,
		uint32_t R1,
		uint32_t R2,
		uint32_t R3,
		uint32_t R4,
		uint32_t R5,
		uint32_t R6,
		uint32_t R7,
		uint32_t R8,
		uint32_t R9,
		uint32_t R10,
		uint32_t R11,
		uint32_t R12,
		uint32_t R13,
		uint32_t R14,
		uint32_t R15
) {
	if (fulltrace) {
	printf("%08x R0:%08x R1:%08x R2:%08x R3:%08x R4:%08x R5:%08x R6:%08x R7:%08x R8:%08x R9:%08x R10:%08x R11:%08x R12:%08x R13:%08x R14:%08x R15:%08x\n", addr, R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, R13, R14, R15);
	}
}

void
execute1(uint32_t in_R15, uint32_t in_R14, uint32_t in_R13, uint32_t in_R0, uint32_t in_R1, uint32_t in_R2, uint32_t in_R3)
{
	uint32_t R0 = 0, R1 = 0, R2 = 0, R3 = 0;
	uint32_t R4 = 0, R5 = 0, R6 = 0, R7 = 0;
	uint32_t R8 = 0, R9 = 0, R10 = 0, R11 = 0;
	uint32_t R12 = 0, R13 = 0, R14 = 0, R15 = 0;
	uint64_t tmp64;
	uint32_t tmpA, tmpB;
	uint32_t C = 0, N = 0, Z = 0, V = 0;

	R15 = in_R15;
	R14 = in_R14;
	R13 = in_R13;
	R0 = in_R0;
	R1 = in_R1;
	R2 = in_R2;
	R3 = in_R3;
dispatch:
	switch (R15) {
#include "c-code.c"
	default:
		if (R15 == in_R14) {
			//printf("done!\n");
			return;
		}
		printf("unknown PC %08x!\n", R15); 
		assert(0);
	}
}
