// #if / #elif / #else / #endif with #[expr] conditions.
#DEBUG 1
#LEVEL 2

#if DEBUG
A: debug-on path
#elif LEVEL == 3
A: high-opt path
#else
A: default path
#endif

#undef DEBUG
#if DEBUG
B: should not appear
#else
B: DEBUG cleared
#endif

#if LEVEL < 3
C: low-opt path
#endif

#if LEVEL >= 3
D: should not appear
#endif

// Combined with NCM-2 fix:
#if 1 && 1
E: 1&&1 reached
#endif

#if 0 || 1
F: 0||1 reached
#endif
