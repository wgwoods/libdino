#ifndef _COMMON_H
#define _COMMON_H 1

#define MAX(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     (_a > _b) ? _a : _b; })
#define MIN(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     (_a < _b) ? _a : _b; })

#endif /* _COMMON_H */
