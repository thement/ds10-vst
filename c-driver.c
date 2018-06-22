#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "ds10.h"

uint8_t *basemem;

uint32_t tmp32;
uint16_t tmp16;
uint8_t tmp8;


uint32_t *MEM32(uint32_t addr) { return vaddr(addr, 4, 0); }
uint16_t *MEM16(uint32_t addr) { return vaddr(addr, 2, 0); }
uint8_t *MEM8(uint32_t addr) { return vaddr(addr, 1, 0); }

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
#include "c-code.h"
	default:
		if (R15 == in_R14) {
			//printf("done!\n");
			return;
		}
		printf("unknown PC %08x!\n", R15); 
		assert(0);
	}
}
