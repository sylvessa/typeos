#ifndef TYPEOS_MATH_H
#define TYPEOS_MATH_H

#define HUGE_VAL ((double)1e308)
#define NAN ((double)0.0 / 0.0)
#define INFINITY ((double)__builtin_inf())

#define M_E 2.7182818284590452354
#define M_PI 3.14159265358979323846
#define M_PI_2 1.57079632679489661923
#define M_PI_4 0.78539816339744830962
#define M_SQRT1_2 0.70710678118654752440
#define M_SQRT2 1.41421356237309504880
#define M_LN2 0.69314718055994530942
#define M_LN10 2.30258509299404568402
#define M_LOG2E 1.44269504088896340736
#define M_LOG10E 0.43429448190325182765

int __isnan(double x);
int __isinf(double x);
int __isfinite(double x);
int __signbit(double x);

#define isnan(x) __isnan((double)(x))
#define isinf(x) __isinf((double)(x))
#define isfinite(x) __isfinite((double)(x))
#define signbit(x) __signbit((double)(x))

double fabs(double x);
double floor(double x);
double ceil(double x);
double lrint(double x);
double sqrt(double x);
double acos(double x);
double asin(double x);
double atan(double x);
double atan2(double y, double x);
double cos(double x);
double exp(double x);
double log(double x);
double log2(double x);
double log10(double x);
double log1p(double x);
double expm1(double x);
double pow(double x, double y);
double sin(double x);
double tan(double x);
double trunc(double x);
double cosh(double x);
double sinh(double x);
double tanh(double x);
double acosh(double x);
double asinh(double x);
double atanh(double x);
double cbrt(double x);
double hypot(double x, double y);
double fmin(double x, double y);
double fmax(double x, double y);
double fmod(double x, double y);
double remainder(double x, double y);
double fma(double x, double y, double z);
double ldexp(double x, int n);
double frexp(double x, int* exp);
double round(double x);

#endif