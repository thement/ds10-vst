#ifndef DS10_H
#define DS10_H
#include "resampler.h"
#define DS10_BUF_NSAMP (0x40)

#include <stdint.h>
enum {
	DS10_SYNTH1,
	DS10_SYNTH2,
	DS10_DRUM1,
	DS10_DRUM2,
	DS10_DRUM3,
	DS10_DRUM4,
	DS10_MIXER,
	DS10_N_DEVICES,
};

#define INIT_SP 0x01FFA000
#define INIT_LR 0xe0000000

enum {
	MaxPressed = 16,
	MaxVoices = 4,
};

typedef struct Pressed Pressed;
typedef struct MemState MemState;
typedef struct Ds10State Ds10State;
typedef struct DsDevice DsDevice;


struct Pressed {
	int note, velocity;
	int has_voice, voice_id;
};

struct MemState {
	uint8_t *basemem;
	int32_t divregs[16];
	uint32_t divcnt;
	int32_t sqrtregs[3];
	uint32_t sqrtcnt;
};

struct Ds10State {
	MemState mem_state[MaxVoices];
	uint8_t *ds10_mem[MaxVoices];
	double sample_buf[DS10_BUF_NSAMP];
	int in_buf, in_ptr;
	int ds10_polyphony;

	Pressed pressed[MaxPressed];
	int used_voices[MaxVoices];
	double volume[MaxVoices];
	int last_used, n_pressed;

	Resampler resampler;
};

struct DsDevice {
	uint32_t addr, buf_addr;
	uint32_t queue_off;
	uint32_t playback_fn, knob_fn;
};


#define vaddr(ms,x,a,b) ((x) < 0x02800000 ? &(ms)->basemem[((x) - 0x01ff8000) & 0x007fffff]: fault_handler(ms,x,a))

void execute1(MemState *ms, uint32_t in_R15, uint32_t in_R14, uint32_t in_R13, uint32_t in_R0, uint32_t in_R1, uint32_t in_R2, uint32_t in_R3);

double ds10_get_sample(Ds10State *dss);
double ds10_get_sample0(Ds10State *dss);
void ds10_knob(Ds10State *dss, int voice, unsigned id, unsigned val);
void ds10_knob_all(Ds10State *dss, unsigned id, unsigned val);
void ds10_noteon(Ds10State *dss, int note, int velocity);
void ds10_noteoff(Ds10State *dss, int note);
void ds10_set_polyphony(Ds10State *dst, int polyphony);
void ds10_exit(Ds10State *dst);
void ds10_set_resampler(Ds10State *dss, int oversample, double sampling_rate, double extra_ratio);

Ds10State *ds10_init(void);

void *fault_handler(MemState *ms, uint32_t addr, uint32_t len);

#define printf printf2
void print2_init(void);
int printf2(const char *format, ...);


#endif