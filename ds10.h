#define DS10_BUF_NSAMP (0x40)

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

void ds10_synth(int dev_id, int16_t *ptr, int size);
void ds10_knob(int dev_id, unsigned id, unsigned val);
void ds10_noteon(int dev_id, int key, int vel);
void ds10_noteoff(int dev_id);
void ds10_init(int emulate);
void ds10_exit(void);
void ds10_load_sample(int dev_id, const char *path);
void play(int16_t x);
void playblock(unsigned addr, unsigned size);
uint32_t readl(uint32_t addr);
void writel(uint32_t addr, uint32_t val);
void ds10_reverse(void);
