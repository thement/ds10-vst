#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include "ds10.h"


int fulltrace = 0;
void (*execute_fn)(uint32_t pc, uint32_t lr, uint32_t sp, uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3);

void
writel(uint32_t addr, uint32_t val)
{
	*(uint32_t *)vaddr(addr, 4, 0) = val;
}

uint32_t
readl(uint32_t addr)
{
	return *(uint32_t *)vaddr(addr, 4, 0);
}

typedef struct DsDevice DsDevice;

struct DsDevice {
	uint32_t addr, buf_addr;
	uint32_t queue_off;
	uint32_t playback_fn, knob_fn;
};

static DsDevice devices[DS10_N_DEVICES] = {
	{ 0x022489A0, 0x01FF8820, 0x80, 0x02074720, 0x02075648 },
	{ 0x02248FE0, 0x01FF88A0, 0x80, 0x02074720, 0x02075648 },
	{ 0x02249620, 0x01FF8920, 0x80, 0x020722b8, 0 },
	{ 0x02249900, 0x01FF89A0, 0x80, 0x020722b8, 0 },
	{ 0x02249BE0, 0x01FF8A20, 0x80, 0x020722b8, 0 },
	{ 0x02249EC0, 0x01FF8AA0, 0x80, 0x020722b8, 0 },
	{ 0x0224A1A0, 0x02248680, 0xC0, 0x02072e8c, 0 },
};


#define INIT_SP 0x01FFA000
#define INIT_LR 0xe0000000

static int16_t sample_buf[DS10_BUF_NSAMP];
static int in_buf = 0, in_ptr = 0;

int16_t
ds10_get_sample(void)
{
	DsDevice *dev = &devices[0];

	if (in_ptr >= in_buf) {
		int size = sizeof(sample_buf);
		execute_fn(dev->playback_fn, INIT_LR, INIT_SP, dev->addr, 0, 0, 0);
		memcpy(sample_buf, vaddr(dev->buf_addr, size, 0), size);
		in_ptr = 0;
		in_buf = DS10_BUF_NSAMP;
	}
	return sample_buf[in_ptr++];
}


void
ds10_knob(int dev_id, unsigned id, unsigned val)
{
	DsDevice *dev = &devices[dev_id];

	if (dev->knob_fn != 0) {
		execute_fn(dev->knob_fn, INIT_LR, INIT_SP, dev->addr, id, val, 0);
	}
}

void
ds10_noteon(int dev_id, int key, int vel)
{
	DsDevice *dev = &devices[dev_id];

	writel(dev->addr + dev->queue_off, 2);
	writel(dev->addr + dev->queue_off + 4, key);
	writel(dev->addr + 8, 1);
}

void
ds10_noteoff(int dev_id)
{
	DsDevice *dev = &devices[dev_id];

	writel(dev->addr + dev->queue_off, 1);
	writel(dev->addr + dev->queue_off + 4, 0);
	writel(dev->addr + 8, 1);
}

void
ds10_reverse(void)
{

}

extern uint8_t *basemem;
#define SEGSIZE (0x8000 + 0x400000)
static void *
readseg(const char *path)
{
	FILE *fp = fopen(path, "rb");
	void *mem;
	printf("fopen of %s = %p\n", path, fp);
	assert(fp);
	mem = calloc(1, SEGSIZE);
	assert(mem);
	size_t ret = fread(mem, SEGSIZE, 1, fp);
	printf("ret = %d\n", (int)ret);
	assert(ret == 1);
	fclose(fp);
	return mem;
}

void
ds10_init(int emulate)
{
#if 0
	readseg("02000000.bin");
	readseg("01ff8000.bin");
	/* this is our "stack" and scratchpad */
	newseg(0xf0000000, 0x00400000, seg_count++);
#else
	basemem = readseg("c:\\01ff8000x.bin");
#endif

	execute_fn = execute1;
}

void
ds10_exit(void)
{
}

/* utils */

int printf2(const char *format, ...)
{
	char str[1024];

	va_list argptr;
	va_start(argptr, format);
	int ret = vsnprintf(str, sizeof(str), format, argptr);
	va_end(argptr);

	OutputDebugStringA(str);

	return ret;
}
