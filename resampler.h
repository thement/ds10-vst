#define NZEROS 4
#define NPOLES 4
#define GAIN   1.481376007e+01

typedef struct Filterx2 Filterx2;
struct Filterx2 {
	double xv[NZEROS + 1], yv[NPOLES + 1];
};

/* must be able to hold input buffer size*4 + 1 */
#define RBUF_SIZE 512

typedef struct Resampler Resampler;
struct Resampler {
	int oversample;
	Filterx2 f1, f2;
	unsigned r, w;
	double t, phase_inc;
	double data[RBUF_SIZE];
};

void resamp_refill(Resampler *r, double *in, int len);
int resamp_get(Resampler *r, double *py);