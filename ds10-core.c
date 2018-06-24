#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include "ds10-core.h"

static void adjust_resampler(Ds10State *dss);

static void
writel(MemState *ms, uint32_t addr, uint32_t val)
{
	*(uint32_t *)vaddr(ms, addr, 4, 0) = val;
}

static uint32_t
readl(MemState *ms, uint32_t addr)
{
	return *(uint32_t *)vaddr(ms, addr, 4, 0);
}

static const DsDevice devices[DS10_N_DEVICES] = {
	{ 0x022489A0, 0x01FF8820, 0x80, 0x02074720, 0x02075648 },
	{ 0x02248FE0, 0x01FF88A0, 0x80, 0x02074720, 0x02075648 },
	{ 0x02249620, 0x01FF8920, 0x80, 0x020722b8, 0 },
	{ 0x02249900, 0x01FF89A0, 0x80, 0x020722b8, 0 },
	{ 0x02249BE0, 0x01FF8A20, 0x80, 0x020722b8, 0 },
	{ 0x02249EC0, 0x01FF8AA0, 0x80, 0x020722b8, 0 },
	{ 0x0224A1A0, 0x02248680, 0xC0, 0x02072e8c, 0 },
};

static void
ds10_render_samplebuf(Ds10State *dss)
{
	const DsDevice *dev = &devices[0];

	memset(dss->sample_buf, 0, sizeof(dss->sample_buf));
	for (int v = 0; v < dss->ds10_polyphony; v++) {
		double vol = dss->volume[v];
		execute1(&dss->mem_state[v], dev->playback_fn, INIT_LR, INIT_SP, dev->addr, 0, 0, 0);
		int16_t *buf = vaddr(&dss->mem_state[v], dev->buf_addr, DS10_BUF_NSAMP * 2, 0);
		for (int i = 0; i < DS10_BUF_NSAMP; i++)
			dss->sample_buf[i] += buf[i] / 32768.0 * vol;
	}
}

double
ds10_get_sample(Ds10State *dss)
{
	if (dss->in_ptr >= dss->in_buf) {
		ds10_render_samplebuf(dss);
		dss->in_ptr = 0;
		dss->in_buf = DS10_BUF_NSAMP;
	}
	return dss->sample_buf[dss->in_ptr++];
}

double
ds10_get_sample0(Ds10State *dss)
{
	double y;
	while (!resamp_get(&dss->resampler, &y)) {
		ds10_render_samplebuf(dss);
		resamp_refill(&dss->resampler, dss->sample_buf, DS10_BUF_NSAMP);
	}
	return y;
}

void
ds10_knob(Ds10State *dss, int voice, unsigned id, unsigned val)
{
	const DsDevice *dev = &devices[0];

	if (voice >= MaxVoices)
		return;

	if (dev->knob_fn != 0) {
		execute1(&dss->mem_state[voice], dev->knob_fn, INIT_LR, INIT_SP, dev->addr, id, val, 0);
	}
}

void
ds10_knob_all(Ds10State *dss, unsigned id, unsigned val)
{
	/* change knob for all voices, in case they became active later */
	for (int i = 0; i < MaxVoices; i++)
		ds10_knob(dss, i, id, val);
}

static double
midi_to_gain(int midi)
{
	double v = (double)midi / 0x7f;
	return v;
}

static void
ds10_noteon_voice(Ds10State *dss, int voice, int key, int vel)
{
	const DsDevice *dev = &devices[0];
	if (voice >= MaxVoices)
		return;
	if (dss->pitch_by_resample)
		key = 36;

	MemState *ms = &dss->mem_state[voice];
	dss->volume[voice] = midi_to_gain(vel);

	writel(ms, dev->addr + dev->queue_off, 2);
	writel(ms, dev->addr + dev->queue_off + 4, key);
	writel(ms, dev->addr + 8, 1);
}

static void
ds10_noteoff_voice(Ds10State *dss, int voice)
{
	const DsDevice *dev = &devices[0];

	if (voice >= MaxVoices)
		return;

	MemState *ms = &dss->mem_state[voice];
	writel(ms, dev->addr + dev->queue_off, 1);
	writel(ms, dev->addr + dev->queue_off + 4, 0);
	writel(ms, dev->addr + 8, 1);
}


// if there's a free voice, use that voice
// otherwise: steal voice that's playing the longest

// stop playing note and release the voice
// if there's a key that has no associated voice, then play that key

static int get_free_voice(Ds10State *dss, int *pv)
{
	int v = dss->last_used;
	for (int i = 0; i < dss->ds10_polyphony; i++) {
		v = (v + 1) % dss->ds10_polyphony;
		if (dss->used_voices[v] == 0) {
			dss->used_voices[v] = 1;
			dss->last_used = v;
			*pv = v;
			return 1;
		}
	}
	return 0;
}

static void return_unused_voice(Ds10State *dss, int v)
{
	assert(dss->used_voices[v]);
	dss->used_voices[v] = 0;
}

static void pr_assign_voice(Ds10State *dss, Pressed *pr, int v)
{
	pr->voice_id = v;
	ds10_noteon_voice(dss, pr->voice_id, pr->note, pr->velocity);
	pr->has_voice = 1;
}

static void pr_steal_voice(Ds10State *dss, Pressed *pr, int *pv)
{
	assert(pr->has_voice);
	ds10_noteoff_voice(dss, pr->voice_id);
	*pv = pr->voice_id;
	pr->voice_id = -1;
	pr->has_voice = 0;
}

int
translate_note(Ds10State *dss, int *note)
{
	/* detect if we are pitching by resampling */
	if (*note < 0) {
		dss->pitch_by_resample = 1;
		*note = -*note;
		dss->resample_note = *note;
	}
	else {
		dss->pitch_by_resample = 0;
	}
	/* shift two octaves down */
	*note -= 24;
	if (note < 0)
		return 0;
	return 1;
}

void
ds10_noteon(Ds10State *dss, int note, int velocity)
{
	int v = -1;

	if (!translate_note(dss, &note))
		return;

	/* ignore noteons if we reached a limit */
	if (dss->n_pressed >= MaxPressed)
		return;
	/* try to get a free voice */
	if (!get_free_voice(dss, &v)) {
		/* steal voice that's been playing the longest */
		for (int i = 0; i < dss->n_pressed; i++) {
			Pressed *pr = &dss->pressed[i];
			if (pr->has_voice) {
				pr_steal_voice(dss, pr, &v);
				break;
			}
		}
		assert(v != -1);
	}
	/* track this key as pressed */
	Pressed *pr = &dss->pressed[dss->n_pressed++];
	pr->note = note;
	pr->velocity = velocity;
	pr_assign_voice(dss, pr, v);

	adjust_resampler(dss);
}

void
ds10_noteoff(Ds10State *dss, int note)
{
	int recyclable_voices = 0;

	if (!translate_note(dss, &note))
		return;
	/* find a voice holding this key */
	for (int i = dss->n_pressed - 1; i >= 0; i--) {
		Pressed *pr = &dss->pressed[i];
		if (pr->note == note) {
			/* if this pressed key is playing on a voice, stop it */
			if (pr->has_voice) {
				int v;
				pr_steal_voice(dss, pr, &v);
				return_unused_voice(dss, v);
				recyclable_voices++;
			}
			/* remove pressed key from table */
			memcpy(&dss->pressed[i], &dss->pressed[i + 1], sizeof(Pressed)*(dss->n_pressed - i - 1));
			dss->n_pressed--;
		}
	}
	/* allocate remaining voices to pressed keys that are not playing */
	for (int i = dss->n_pressed - 1; recyclable_voices > 0 && i >= 0; i--) {
		Pressed *pr = &dss->pressed[i];
		if (!pr->has_voice) {
			int v;
			if (!get_free_voice(dss, &v))
				assert(0);
			pr_assign_voice(dss, pr, v);
			recyclable_voices--;
		}
	}
}

#define SEGSIZE (0x8000 + 0x400000)
#if 0
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
#else
extern unsigned char __01ff8000x_bin[];
static void *
readseg(const char *path)
{
	void *mem = calloc(1, SEGSIZE);
	assert(mem);
	memcpy(mem, __01ff8000x_bin, SEGSIZE);
	return mem;
}
#endif

void
ds10_set_polyphony(Ds10State *dst, int polyphony)
{
	if (polyphony >= MaxVoices)
		polyphony = MaxVoices;
	if (polyphony < 1)
		polyphony = 1;
	dst->ds10_polyphony = polyphony;

	printf("polyphony set to %d\n", dst->ds10_polyphony);
}

void
ds10_exit(Ds10State *dss)
{
	for (int i = 0; i < MaxVoices; i++)
		free(dss->mem_state[i].basemem);
	free(dss);
}

static void
adjust_resampler(Ds10State *dss)
{
	double ratio = dss->ratio;
	if (dss->pitch_by_resample)
		ratio *= pow(2, (dss->resample_note - 60) / 12.0);
	resamp_reset(&dss->resampler, dss->oversample, ratio);
}

void
ds10_set_resampler(Ds10State *dss, int oversample, double sampling_rate, double extra_ratio)
{
	dss->oversample = oversample;
	dss->ratio = 32768.0 / sampling_rate * extra_ratio;
	adjust_resampler(dss);
	printf("over=%d rate=%lf extra_ratio=%lf\n", oversample, sampling_rate, extra_ratio);
}

Ds10State *
ds10_init(void)
{
	Ds10State *dss = calloc(1, sizeof(*dss));

	ds10_set_resampler(dss, 1, 44100, 1);

	for (int i = 0; i < MaxVoices; i++) {
		dss->mem_state[i].basemem = readseg("c:\\01ff8000x.bin");
	}
	dss->ds10_polyphony = 1;

	return dss;
}


/* utils */
static FILE *xlog = 0;

void print2_init(void)
{
#if 0
	if (!xlog)
		xlog = fopen("C:\\Users\\Poly Cajt\\log.txt", "w");
#endif
}

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

