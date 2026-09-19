#include <math.h>
#include <stdint.h>
typedef union {
	double d;
	uint64_t u;
} dbits_t;

#define EXP_MASK 0x7FF0000000000000ULL
#define MAN_MASK 0x000FFFFFFFFFFFFFULL

int __isnan(double x) {
	dbits_t c;
	c.d = x;
	return (c.u & EXP_MASK) == EXP_MASK && (c.u & MAN_MASK) != 0;
}

int __isinf(double x) {
	dbits_t c;
	c.d = x;
	return (c.u & EXP_MASK) == EXP_MASK && (c.u & MAN_MASK) == 0;
}

int __isfinite(double x) {
	dbits_t c;
	c.d = x;
	return (c.u & EXP_MASK) != EXP_MASK;
}

int __signbit(double x) {
	dbits_t c;
	c.d = x;
	return (int)(c.u >> 63);
}

double fabs(double x) {
	double r;
	__asm__("fabs" : "=t"(r) : "0"(x));
	return r;
}

double sqrt(double x) {
	double r;
	__asm__("fsqrt" : "=t"(r) : "0"(x));
	return r;
}

double sin(double x) {
	double r;
	__asm__("fsin" : "=t"(r) : "0"(x));
	return r;
}

double cos(double x) {
	double r;
	__asm__("fcos" : "=t"(r) : "0"(x));
	return r;
}

double tan(double x) {
	double r;
	__asm__("fptan\n\tfxch\n\tfstp %%st(1)" : "=t"(r) : "0"(x));
	return r;
}

double atan(double x) {
	double r;
	__asm__("fld1\n\tfpatan" : "=t"(r) : "0"(x));
	return r;
}

double atan2(double y, double x) {
	double r;
	__asm__("fpatan" : "=t"(r) : "0"(x), "u"(y));
	return r;
}

double exp(double x) {
	double r;
	__asm__("fldl2e\n\t"
			"fmulp %%st, %%st(1)\n\t"
			"fld %%st(0)\n\t"
			"frndint\n\t"
			"fsub %%st, %%st(1)\n\t"
			"fxch\n\t"
			"f2xm1\n\t"
			"fld1\n\t"
			"faddp %%st, %%st(1)\n\t"
			"fscale\n\t"
			"fstp %%st(1)"
			: "=t"(r)
			: "0"(x));
	return r;
}

double log2(double x) {
	double r;
	__asm__("fld1\n\tfxch\n\tfyl2x" : "=t"(r) : "0"(x));
	return r;
}

double log(double x) {
	double r;
	__asm__("fldln2\n\tfxch\n\tfyl2x" : "=t"(r) : "0"(x));
	return r;
}

double log10(double x) {
	double r;
	__asm__("fldl2t\n\tfxch\n\tfyl2x" : "=t"(r) : "0"(x));
	return r;
}

double log1p(double x) { return log(1.0 + x); }
double expm1(double x) { return exp(x) - 1.0; }

double pow(double x, double y) {
	if (x == 1.0 || y == 0.0)
		return 1.0;
	if (x > 0.0)
		return exp(y * log(x));
	if (x == 0.0)
		return y > 0.0 ? 0.0 : NAN;
	return NAN; // negative base
}

double fmod(double x, double y) {
	double r;
	__asm__("1:\n\t"
			"fprem\n\t"
			"fnstsw %%ax\n\t"
			"sahf\n\t"
			"jp 1b"
			: "=t"(r)
			: "0"(x), "u"(y));
	return r;
}

double remainder(double x, double y) {
	double r;
	__asm__("1:\n\t"
			"fprem1\n\t"
			"fnstsw %%ax\n\t"
			"sahf\n\t"
			"jp 1b"
			: "=t"(r)
			: "0"(x), "u"(y));
	return r;
}

static uint16_t fp_cw(void) {
	uint16_t cw;
	__asm__("fnstcw %0" : "=m"(cw));
	return cw;
}

static void fp_cw_set(uint16_t cw) { __asm__("fldcw %0" : : "m"(cw)); }

static double frndint_mode(double x, uint16_t rc) {
	uint16_t old = fp_cw();
	fp_cw_set((uint16_t)((old & ~0x0C00) | rc));
	double r;
	__asm__("frndint" : "=t"(r) : "0"(x));
	fp_cw_set(old);
	return r;
}

double floor(double x) { return frndint_mode(x, 0x0400); }
double ceil(double x) { return frndint_mode(x, 0x0800); }
double trunc(double x) { return frndint_mode(x, 0x0C00); }
double round(double x) { return frndint_mode(x, 0x0000); }

double lrint(double x) {
	long r;
	__asm__("frndint\n\tfistpl %0" : "=m"(r) : "t"(x));
	return (double)r;
}

double asin(double x) {
	double s = sqrt(1.0 - x * x);
	return atan2(x, s);
}

double acos(double x) {
	if (x == 0.0)
		return M_PI / 2.0;
	double s = sqrt(1.0 - x * x);
	return atan2(s, x);
}

double cosh(double x) {
	double e = exp(x);
	return (e + 1.0 / e) * 0.5;
}

double sinh(double x) {
	double e = exp(x);
	return (e - 1.0 / e) * 0.5;
}

double tanh(double x) {
	double e = exp(2.0 * x);
	return (e - 1.0) / (e + 1.0);
}

double asinh(double x) { return log(x + sqrt(x * x + 1.0)); }

double acosh(double x) { return log(x + sqrt(x - 1.0) * sqrt(x + 1.0)); }

double atanh(double x) { return 0.5 * log((1.0 + x) / (1.0 - x)); }

double cbrt(double x) {
	if (x == 0.0 || __isinf(x))
		return x;
	double y = __signbit(x) ? -exp(log(-x) / 3.0) : exp(log(x) / 3.0);
	return y;
}

double hypot(double x, double y) {
	x = fabs(x);
	y = fabs(y);
	if (x == 0.0)
		return y;
	if (y == 0.0)
		return x;
	double hi = x > y ? x : y;
	double lo = x > y ? y : x;
	double t = lo / hi;
	return hi * sqrt(1.0 + t * t);
}

double fma(double x, double y, double z) { return x * y + z; }

double fmin(double x, double y) { return x < y ? x : y; }
double fmax(double x, double y) { return x > y ? x : y; }

double ldexp(double x, int n) {
	double r;
	__asm__("fscale" : "=t"(r) : "0"(x), "u"((double)(long long)n));
	return r;
}

double frexp(double x, int* exp) {
	if (x == 0.0 || !__isfinite(x)) {
		*exp = 0;
		return x;
	}
	*exp = 0;
	double ax = fabs(x);
	while (ax >= 1.0) {
		ax *= 0.5;
		++*exp;
	}
	while (ax < 0.5) {
		ax *= 2.0;
		--*exp;
	}
	return __signbit(x) ? -ax : ax;
}