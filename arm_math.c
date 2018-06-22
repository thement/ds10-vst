#include <stdint.h>
#include <math.h>
#include "ds10.h"

static int32_t divregs[16];
static uint32_t divcnt;
static int32_t sqrtregs[3];
static uint32_t sqrtcnt;

void *
fault_handler(uint32_t addr, uint32_t len)
{
	if (addr >= 0x4000290 && addr < 0x40002b0) {
		if (len != 4)
			return 0;
		return &divregs[(addr - 0x4000290) / 4];
	}
	if (addr >= 0x40002b4 && addr < 0x40002c0) {
		if (len != 4)
			return 0;
		return &sqrtregs[(addr - 0x40002b4) / 4];
	}

	if (addr == 0x4000280) {
		/* do calculation, return status */
		int64_t numer, denom;

		switch (divcnt & 3) {
			case 0:
				numer = divregs[0];
				denom = divregs[2];
				break;
			case 1:
				numer = (int64_t)divregs[1] << 32;
				numer |= (uint32_t)divregs[0];
				denom = divregs[2];
				break;
			case 2:
				numer = (int64_t)divregs[1] << 32;
				numer |= (uint32_t)divregs[0];
				denom = (int64_t)divregs[3] << 32;
				denom |= (uint32_t)divregs[2];
				break;
			default:
				numer = denom = 0;
				break;
		}
		divcnt &= 3;
		if (denom == 0) {
			divcnt |= 0x4000;
		} else {
			int64_t quot = numer / denom;
			int64_t rem = numer % denom;

			divregs[4] = quot;
			divregs[5] = quot >> 32;
			divregs[6] = rem;
			divregs[7] = rem >> 32;
			//printf("division %llx / %llx = %llx rem %llx\n", (unsigned long long) numer, (unsigned long long) denom, quot, rem);
		}
		return &divcnt;
	}
	if (addr == 0x40002b0) {
		uint64_t val;
		val = (uint64_t)sqrtregs[2] << 32;
		val |= sqrtregs[1];
		sqrtregs[0] = sqrt(val);
		sqrtcnt &= 1;
		//printf("sqrt: %lx -> %x\n", val, sqrtregs[0]);
		return &sqrtcnt;
	}
	return 0;
}


