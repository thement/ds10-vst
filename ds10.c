#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include "dat.h"
#include "fns.h"
#include "ds10.h"


static Process static_P;
static FILE *out;
Process *P = &static_P;
int fulltrace = 0;
void (*execute_fn)(u32int pc, u32int lr, u32int sp, u32int r0, u32int r1, u32int r2, u32int r3);

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

void
play(int16_t x)
{
	if (out)
		fwrite(&x, 2, 1, out);
}

void
playblock(unsigned addr, unsigned size)
{
	for (int i = 0; i < size; i++) {
		play(*(uint16_t *)vaddr(addr + 2*i, 2, 0));
	}
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


/* FIXME */
typedef struct Data Data;
struct Data {
	void *data;
	size_t len;
};

void
ds10_load_sample(int dev_id, const char *path)
{
	uint32_t sample_addr, sample_len;
	void *sample_mem;
	DsDevice *dev = &devices[dev_id];

	if (!(dev_id >= DS10_DRUM1 && dev_id <= DS10_DRUM4))
		return;

	sample_len = readl(dev->addr + 0x40);
	sample_addr = readl(dev->addr + 0x44);
	assert(sample_len == 0x8000);
	sample_mem = vaddr(sample_addr, sample_len, 0);

	int fd = open(path, O_RDONLY);
	if (fd < 0)
		pexit(path);
	if (read(fd, sample_mem, sample_len) != sample_len) {
		fprintf(stderr, "short read %s\n", path);
		exit(1);
	}
	close(fd);
}
#define INIT_SP 0x01FFA000
#define INIT_LR 0xe0000000
void
ds10_synth(int dev_id, int16_t *ptr, int size)
{
	DsDevice *dev = &devices[dev_id];

	execute_fn(dev->playback_fn, INIT_LR, INIT_SP, dev->addr, 0, 0, 0);
	playblock(dev->buf_addr, DS10_BUF_NSAMP);
	if (ptr) {
		assert(size == DS10_BUF_NSAMP*2);
		memcpy(ptr, vaddr(dev->buf_addr, size, 0), size);
	}
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
	reverse();
}

extern uint8_t *basemem;

void
ds10_init(int emulate)
{
#if 0
	readseg("02000000.bin");
	readseg("01ff8000.bin");
	/* this is our "stack" and scratchpad */
	newseg(0xf0000000, 0x00400000, seg_count++);
#else
	readseg("01ff8000x.bin");
	basemem = P->S[0]->data;
#endif

	if (emulate)
		execute_fn = execute;
	else
		execute_fn = execute1;

	out = fopen("out.raw", "wb");
}

void
ds10_exit(void)
{
	printmap(0);
	printmap(1);
	if (out)
		fclose(out);
}
