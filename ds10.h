
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

int16_t ds10_get_sample(void);
void ds10_knob(int dev_id, unsigned id, unsigned val);
void ds10_noteon(int key, int vel);
void ds10_noteoff(void);
void ds10_init(int emulate);
void ds10_exit(void);
uint32_t readl(uint32_t addr);
void writel(uint32_t addr, uint32_t val);
void ds10_reverse(void);

void *fault_handler(uint32_t addr, uint32_t len);

#define printf printf2
int printf2(const char *format, ...);
extern uint8_t *basemem;

#define vaddr(x,a,b) ((x) < 0x02800000 ? &basemem[((x) - 0x01ff8000) & 0x007fffff]: fault_handler(x,a))

void execute1(uint32_t in_R15, uint32_t in_R14, uint32_t in_R13, uint32_t in_R0, uint32_t in_R1, uint32_t in_R2, uint32_t in_R3);