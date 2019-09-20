#ifndef _COMMON_H
#define _COMMON_H 1

#include <limits.h>

#define MAX(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     (_a > _b) ? _a : _b; })
#define MIN(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     (_a < _b) ? _a : _b; })
#define CLAMP(a,min,max) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (min) _min = (min); \
       __typeof__ (max) _max = (max); \
     (_a < _max) ? ((_a > _min) ? _a : _min) : _max; })

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

#define bitsizeof(x) (CHAR_BIT * sizeof(x))
#define MSB(x, bits) ((x) & (~0ULL << (bitsizeof(x) - (bits))))

#endif /* _COMMON_H */
