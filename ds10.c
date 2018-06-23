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

#define MAX_VOICES 4

static uint8_t *ds10_mem[MAX_VOICES];
static double sample_buf[DS10_BUF_NSAMP];
static int in_buf = 0, in_ptr = 0;
static int ds10_polyphony = 0;

double
ds10_get_sample(void)
{
	DsDevice *dev = &devices[0];

	if (in_ptr >= in_buf) {
		memset(sample_buf, 0, sizeof(sample_buf));
		for (int v = 0; v < ds10_polyphony; v++) {
			basemem = ds10_mem[v];
			execute_fn(dev->playback_fn, INIT_LR, INIT_SP, dev->addr, 0, 0, 0);
			int16_t *buf = vaddr(dev->buf_addr, DS10_BUF_NSAMP * 2, 0);
			for (int i = 0; i < DS10_BUF_NSAMP; i++)
				sample_buf[i] += buf[i] / 32768.0;
		}
		in_ptr = 0;
		in_buf = DS10_BUF_NSAMP;
	}
	return sample_buf[in_ptr++];
}


void
ds10_knob(int voice, unsigned id, unsigned val)
{
	DsDevice *dev = &devices[0];

	if (voice >= MAX_VOICES)
		return;

	basemem = ds10_mem[voice];
	if (dev->knob_fn != 0) {
		execute_fn(dev->knob_fn, INIT_LR, INIT_SP, dev->addr, id, val, 0);
	}
}

void
ds10_knob_all(unsigned id, unsigned val)
{
	/* change knob for all voices, in case they became active later */
	for (int i = 0; i < MAX_VOICES; i++)
		ds10_knob(i, id, val);
}

static void
ds10_noteon_voice(int voice, int key, int vel)
{
	DsDevice *dev = &devices[0];
	if (voice >= MAX_VOICES)
		return;
	basemem = ds10_mem[voice];
	writel(dev->addr + dev->queue_off, 2);
	writel(dev->addr + dev->queue_off + 4, key);
	writel(dev->addr + 8, 1);
}

static void
ds10_noteoff_voice(int voice)
{
	DsDevice *dev = &devices[0];
	if (voice >= MAX_VOICES)
		return;
	basemem = ds10_mem[voice];
	writel(dev->addr + dev->queue_off, 1);
	writel(dev->addr + dev->queue_off + 4, 0);
	writel(dev->addr + 8, 1);
}

enum {
	MaxPressed = 16,
	MaxVoices = MAX_VOICES,
};
typedef struct Pressed Pressed;
struct Pressed {
	int note, velocity;
	int has_voice, voice_id;
};

static Pressed pressed[MaxPressed];
static int used_voices[MaxVoices];
int last_used = 0, n_pressed = 0, n_playing = 0;

// if there's a free voice, use that voice
// otherwise: steal voice that's playing the longest

// stop playing note and release the voice
// if there's a key that has no associated voice, then play that key

static int get_free_voice(int *pv)
{
	int v = last_used;
	for (int i = 0; i < ds10_polyphony; i++) {
		v = (v + 1) % ds10_polyphony;
		if (used_voices[v] == 0) {
			used_voices[v] = 1;
			last_used = v;
			*pv = v;
			return 1;
		}
	}
	return 0;
}

static void return_unused_voice(int v)
{
	assert(used_voices[v]);
	used_voices[v] = 0;
}

static void pr_assign_voice(Pressed *pr, int v)
{
	pr->voice_id = v;
	ds10_noteon_voice(pr->voice_id, pr->note, pr->velocity);
	pr->has_voice = 1;
}

static void pr_steal_voice(Pressed *pr, int *pv)
{
	assert(pr->has_voice);
	ds10_noteoff_voice(pr->voice_id);
	*pv = pr->voice_id;
	pr->voice_id = -1;
	pr->has_voice = 0;
}

void
ds10_noteon(int note, int velocity)
{
	int v = -1;
	/* ignore noteons if we reached a limit */
	if (n_pressed >= MaxPressed)
		return;
	/* try to get a free voice */
	if (!get_free_voice(&v)) {
		/* steal voice that's been playing the longest */
		for (int i = 0; i < n_pressed; i++) {
			Pressed *pr = &pressed[i];
			if (pr->has_voice) {
				pr_steal_voice(pr, &v);
				break;
			}
		}
		assert(v != -1);
	}
	/* track this key as pressed */
	Pressed *pr = &pressed[n_pressed++];
	pr->note = note;
	pr->velocity = velocity;
	pr_assign_voice(pr, v);
}

void
ds10_noteoff(int note)
{
	int recyclable_voices = 0;
	/* find a voice holding this key */
	for (int i = n_pressed - 1; i >= 0; i--) {
		Pressed *pr = &pressed[i];
		if (pr->note == note) {
			/* if this pressed key is playing on a voice, stop it */
			if (pr->has_voice) {
				int v;
				pr_steal_voice(pr, &v);
				return_unused_voice(v);
				recyclable_voices++;
			}
			/* remove pressed key from table */
			memcpy(&pressed[i], &pressed[i + 1], sizeof(Pressed)*(n_pressed - i - 1));
			n_pressed--;
		}
	}
	/* allocate remaining voices to pressed keys that are not playing */
	for (int i = n_pressed - 1; recyclable_voices > 0 && i >= 0; i--) {
		Pressed *pr = &pressed[i];
		if (!pr->has_voice) {
			int v;
			if (!get_free_voice(&v))
				assert(0);
			pr_assign_voice(pr, v);
			recyclable_voices--;
		}
	}
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
FILE *xlog;
void
ds10_set_polyphony(int polyphony)
{
	if (polyphony >= MAX_VOICES)
		polyphony = MAX_VOICES;
	if (polyphony < 1)
		polyphony = 1;
	ds10_polyphony = polyphony;

	printf("polyphont set to %d\n", ds10_polyphony);
}

void
ds10_init(void)
{
	xlog = fopen("C:\\Users\\Poly Cajt\\log.txt", "w");
#if 0
	readseg("02000000.bin");
	readseg("01ff8000.bin");
	/* this is our "stack" and scratchpad */
	newseg(0xf0000000, 0x00400000, seg_count++);
#else
	for (int i = 0; i < MAX_VOICES; i++)
		ds10_mem[i] = readseg("c:\\01ff8000x.bin");
	ds10_polyphony = 1;
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
	if (xlog) {
		fputs(str, xlog);
		fflush(xlog);
	}

	return ret;
}
