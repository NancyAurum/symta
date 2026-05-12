#ifndef NCU_DEFS_H
#define NCU_DEFS_H

#define SWAP(x,y) do {int t_ = x; x = y; y = t_;} while(0)
#define SWAPD(x,y) do {double t_ = x; x = y; y = t_;} while(0)
#define SWAPT(t,x,y) do {t t_ = x; x = y; y = t_;} while(0)

#define MIN(x,y) ((x)<(y)?(x):(y))
#define MAX(x,y) ((x)>(y)?(x):(y))

//declaring a pointer RESTRICT, we guarantee compiler,
//that no other pointer will be used to modify the pointed data.
//so compiler could keep more stuff in memory,
//instead of immediately writing back.
//#define RESTRICT __restrict__
#define RESTRICT restrict
//#define RESTRICT 

//inline forces compiler to expand the function into callee.
#define INLINE static __attribute__((always_inline)) inline
//#define INLINE static inline

#define NOINLINE __attribute__ ((noinline))

#define ALIGN(N) __attribute__ ((aligned(N)))

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

#define PACKED __attribute__((packed))

#endif

