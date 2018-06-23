#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "ds10.h"

static double run_filter(Filterx2 *fx2, double x)
{
	double *xv = fx2->xv, *yv = fx2->yv;

	xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4];
	xv[4] = x / GAIN;
	yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4];
	yv[4] = (xv[0] + xv[4]) + 4 * (xv[1] + xv[3]) + 6 * xv[2]
		+ (-0.0206514245 * yv[0]) + (0.0842337122 * yv[1])
		+ (-0.5343006371 * yv[2]) + (0.3906414532 * yv[3]);
	return yv[4];
}

static inline unsigned readable(Resampler *r)
{
	return (r->w - r->r) % RBUF_SIZE;
}

static inline unsigned writable(Resampler *r)
{
	return RBUF_SIZE - (r->w - r->r) % RBUF_SIZE - 1;
}

static inline void drop(Resampler *r, unsigned n)
{
	r->r = (r->r + n) % RBUF_SIZE;
}

static inline void putbuf(Resampler *r, double x)
{
	r->data[r->w] = x;
	r->w = (r->w + 1) % RBUF_SIZE;
}

static inline double getbuf(Resampler *r)
{
	double x = r->data[r->r];
	r->r = (r->r + 1) % RBUF_SIZE;
	return x;
}

void resamp_refill(Resampler *r, double *in, int len)
{
	assert(writable(r) >= len*4);
	for (int i = 0; i < len; i++) {
		double y0 = run_filter(&r->f1, in[i]);
		putbuf(r, run_filter(&r->f2, y0));
		putbuf(r, run_filter(&r->f2, 0));
		double y1 = run_filter(&r->f1, 0);
		putbuf(r, run_filter(&r->f2, y1));
		putbuf(r, run_filter(&r->f2, 0));
	}
}

int resamp_get(Resampler *r, double *py)
{
	int index = (int)r->t;
	int ready = readable(r);

	if (index > 0) {
		if (ready < index) {
			drop(r, ready);
			r->t -= ready;
			return 0;
		}
		drop(r, index);
		r->t -= index;
	}
	assert((int)r->t == 0);
	if (readable(r) < 2)
		return 0;

	double a = r->data[r->r];
	double b = r->data[(r->r + 1) % RBUF_SIZE];
	
	*py = (a*(1 - r->t) + b*r->t)*4;
	r->t += r->phase_inc;
	return 1;
}

#if 0
int
main(int argc, char *argv[])
{
	double data_in[128];
	int16_t raw_in[0x40];
	int n_read;
	Resampler resampler = { .phase_inc = 32768.0/44100*4 };
	double y;

	for (;;) {
		while (!resamp_get(&resampler, &y)) {
			int n = read(0, raw_in, sizeof(raw_in));
			if (n == 0)
				return 0;
			for (int i=0; i<n/2; i++) {
				data_in[i] = raw_in[i];
			}
			resamp_refill(&resampler, data_in, n/2);
		}
		int16_t ys = y;
		write(1, &ys, 2);
	}
	return 0;
}
#endif