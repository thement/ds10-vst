#include <stdint.h>
#include <math.h>
#include "ds10.h"

void *
fault_handler(MemState *ms, uint32_t addr, uint32_t len)
{
	if (addr >= 0x4000290 && addr < 0x40002b0) {
		if (len != 4)
			return 0;
		return &ms->divregs[(addr - 0x4000290) / 4];
	}
	if (addr >= 0x40002b4 && addr < 0x40002c0) {
		if (len != 4)
			return 0;
		return &ms->sqrtregs[(addr - 0x40002b4) / 4];
	}

	if (addr == 0x4000280) {
		/* do calculation, return status */
		int64_t numer, denom;

		switch (ms->divcnt & 3) {
			case 0:
				numer = ms->divregs[0];
				denom = ms->divregs[2];
				break;
			case 1:
				numer = (int64_t)ms->divregs[1] << 32;
				numer |= (uint32_t)ms->divregs[0];
				denom = ms->divregs[2];
				break;
			case 2:
				numer = (int64_t)ms->divregs[1] << 32;
				numer |= (uint32_t)ms->divregs[0];
				denom = (int64_t)ms->divregs[3] << 32;
				denom |= (uint32_t)ms->divregs[2];
				break;
			default:
				numer = denom = 0;
				break;
		}
		ms->divcnt &= 3;
		if (denom == 0) {
			ms->divcnt |= 0x4000;
		} else {
			int64_t quot = numer / denom;
			int64_t rem = numer % denom;

			ms->divregs[4] = quot;
			ms->divregs[5] = quot >> 32;
			ms->divregs[6] = rem;
			ms->divregs[7] = rem >> 32;
			//printf("division %llx / %llx = %llx rem %llx\n", (unsigned long long) numer, (unsigned long long) denom, quot, rem);
		}
		return &ms->divcnt;
	}
	if (addr == 0x40002b0) {
		uint64_t val;
		val = (uint64_t)ms->sqrtregs[2] << 32;
		val |= ms->sqrtregs[1];
		ms->sqrtregs[0] = sqrt(val);
		ms->sqrtcnt &= 1;
		//printf("sqrt: %lx -> %x\n", val, sqrtregs[0]);
		return &ms->sqrtcnt;
	}
	return 0;
}


