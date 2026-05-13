/* cffilib — the C library used by tests/ffi/.
 *
 * A small set of deterministic functions whose return values are
 * exact-checkable from Symta. Every function exercises a specific
 * (return-type, arg-types, arity) combination that we care about
 * for the FFI dispatcher (sffi). The goal is total coverage of
 * the type and arity matrix, not realistic API patterns.
 *
 * Realistic-API tests go in cases/15-zlib-*.s and cases/17-libc-*.s
 * which dynamically load zlib / libc / msvcrt and call existing
 * functions there.
 *
 * Build: per-platform Makefile produces a shared library that
 * Symta loads via `ffi_begin local cffi`. The Symta side maps
 * "cffi" -> "ffi/cffi.ffi" (a renamed .dll / .dylib / .so).
 *
 * Function-naming convention:
 *
 *   c_<purpose>_<rettype>[_<argtype>...]
 *
 * e.g. c_add_i32_i32_i32 = "add two i32, return i32".
 *
 * Type abbreviations used in names:
 *   v    void
 *   i32  int    (sign-extended)
 *   u32  uint32 (zero-extended)
 *   f32  float
 *   f64  double
 *   ptr  void*
 *   str  const char*
 *   buf  void*  (writeable buffer)
 *
 * Licence: same dual MIT / Apache-2 as the rest of Symta.
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#  define CFFI_EXPORT __declspec(dllexport)
#else
#  define CFFI_EXPORT __attribute__((visibility("default")))
#endif

/* ============================================================
 * Identity / round-trip
 * Verifies: the bits go in, the same bits come out.
 * ============================================================ */

CFFI_EXPORT int      c_id_i32(int x)            { return x; }
CFFI_EXPORT uint32_t c_id_u32(uint32_t x)       { return x; }
CFFI_EXPORT float    c_id_f32(float x)          { return x; }
CFFI_EXPORT double   c_id_f64(double x)         { return x; }
CFFI_EXPORT void    *c_id_ptr(void *p)          { return p; }

/* String identity — returns the same pointer the caller passed. */
CFFI_EXPORT const char *c_id_str(const char *s) { return s; }


/* ============================================================
 * Constant returns
 * Verifies: typical sentinel values come back intact.
 * ============================================================ */

CFFI_EXPORT int      c_const_i32_max(void)      { return 2147483647; }
CFFI_EXPORT int      c_const_i32_min(void)      { return -2147483647 - 1; }
CFFI_EXPORT int      c_const_i32_neg1(void)     { return -1; }
CFFI_EXPORT uint32_t c_const_u32_max(void)      { return 0xFFFFFFFFu; }
CFFI_EXPORT float    c_const_f32_pi(void)       { return 3.14159274f; }
CFFI_EXPORT double   c_const_f64_pi(void)       { return 3.141592653589793; }
CFFI_EXPORT void    *c_const_ptr_null(void)     { return (void *)0; }


/* ============================================================
 * Two-arg arithmetic.
 * Verifies: each arg lands in the right register/stack slot for
 * its TYPE — and the type isn't truncated mid-transit.
 * ============================================================ */

CFFI_EXPORT int      c_add_i32_i32(int a, int b)            { return a + b; }
CFFI_EXPORT uint32_t c_add_u32_u32(uint32_t a, uint32_t b)  { return a + b; }
CFFI_EXPORT float    c_add_f32_f32(float a, float b)        { return a + b; }
CFFI_EXPORT double   c_add_f64_f64(double a, double b)      { return a + b; }
CFFI_EXPORT double   c_add_i32_f64(int a, double b)         { return (double)a + b; }
CFFI_EXPORT double   c_add_f64_i32(double a, int b)         { return a + (double)b; }


/* ============================================================
 * Mixed int/float interleaving.
 *
 * Critical for the SysV vs Win64 distinction:
 *   - Win64: position 0..3 use rcx/rdx/r8/r9 OR xmm0..3 based on
 *     type at that POSITION; mixing consumes slots in order.
 *   - SysV: separate pools — ints fill rdi..r9 (6 regs) and
 *     floats fill xmm0..7 (8 regs) independently.
 *
 * If a backend gets the slot-vs-pool semantics wrong, these will
 * return garbage.
 * ============================================================ */

CFFI_EXPORT double c_mix4_iffi(int a, float b, float c, int d) {
  return (double)a + (double)b + (double)c + (double)d;
}
CFFI_EXPORT double c_mix4_fiif(float a, int b, int c, float d) {
  return (double)a + (double)b + (double)c + (double)d;
}
CFFI_EXPORT double c_mix4_ffii(float a, float b, int c, int d) {
  return (double)a + (double)b + (double)c + (double)d;
}
CFFI_EXPORT double c_mix4_iiff(int a, int b, float c, float d) {
  return (double)a + (double)b + (double)c + (double)d;
}

/* 6 args of alternating type. On SysV this fills xmm0..2 + 3 int
 * regs; on Win64 it consumes slots 0..3 (regs) + 2 stack slots. */
CFFI_EXPORT double c_mix6_ifiifi(int a, float b, int c, int d, float e, int f) {
  return (double)a + (double)b + (double)c + (double)d
       + (double)e + (double)f;
}
CFFI_EXPORT double c_mix6_fiiffi(float a, int b, int c, float d, float e, int f) {
  return (double)a + (double)b + (double)c + (double)d
       + (double)e + (double)f;
}


/* ============================================================
 * Arity coverage.
 *
 * Pushes past the register pool to exercise the stack-arg
 * codepath. Win64 has 4 reg slots; SysV has 6 int + 8 float.
 * ============================================================ */

CFFI_EXPORT int c_sum_i32_4(int a, int b, int c, int d) {
  return a + b + c + d;
}
CFFI_EXPORT int c_sum_i32_6(int a, int b, int c, int d, int e, int f) {
  return a + b + c + d + e + f;
}
CFFI_EXPORT int c_sum_i32_8(int a, int b, int c, int d,
                            int e, int f, int g, int h) {
  return a + b + c + d + e + f + g + h;
}
CFFI_EXPORT int c_sum_i32_12(int a, int b, int c, int d,
                             int e, int f, int g, int h,
                             int i, int j, int k, int l) {
  return a + b + c + d + e + f + g + h + i + j + k + l;
}

CFFI_EXPORT double c_sum_f64_4(double a, double b, double c, double d) {
  return a + b + c + d;
}
CFFI_EXPORT double c_sum_f64_6(double a, double b, double c,
                               double d, double e, double f) {
  return a + b + c + d + e + f;
}
CFFI_EXPORT double c_sum_f64_8(double a, double b, double c, double d,
                               double e, double f, double g, double h) {
  return a + b + c + d + e + f + g + h;
}
/* 10 doubles — overflows SysV's xmm0..7 by 2; tests stack-arg
 * passing for doubles. Important because the stack-arg dispatch
 * could conceivably get the float bit-pattern wrong. */
CFFI_EXPORT double c_sum_f64_10(double a, double b, double c, double d,
                                double e, double f, double g, double h,
                                double i, double j) {
  return a + b + c + d + e + f + g + h + i + j;
}


/* ============================================================
 * String / cstring tests
 *
 * Symta's SBC_NFI_TXT (which sffi maps to SFFI_PTR) marshals
 * Symta texts to `char *`. The C side reads them as null-
 * terminated cstrings.
 * ============================================================ */

CFFI_EXPORT int c_strlen(const char *s) {
  int n = 0;
  while (s[n]) n++;
  return n;
}

CFFI_EXPORT int c_strcmp(const char *a, const char *b) {
  while (*a && *a == *b) { a++; b++; }
  return (unsigned char)*a - (unsigned char)*b;
}

/* Returns the offset of the first occurrence of `ch` in `s`,
 * or -1 if not present. */
CFFI_EXPORT int c_strchr_ofs(const char *s, int ch) {
  int n = 0;
  while (s[n]) {
    if ((unsigned char)s[n] == (unsigned char)ch) return n;
    n++;
  }
  return -1;
}

/* Sum the byte values of a string. Catches "did the pointer
 * survive the call?" — if Symta passes the wrong pointer the sum
 * will be garbage. */
CFFI_EXPORT uint32_t c_str_bytesum(const char *s) {
  uint32_t sum = 0;
  while (*s) {
    sum += (unsigned char)*s++;
  }
  return sum;
}


/* ============================================================
 * Buffer tests
 *
 * The buffer protocol: caller allocates raw memory (via
 * ffi_alloc in Symta, or our c_buf_alloc here), passes the
 * pointer to a C function, that function reads / writes through
 * it.
 *
 * c_buf_fill writes a single byte to every position; the Symta
 * caller can then read it back via _ffi_get to verify.
 *
 * c_buf_checksum walks a buffer the caller filled and returns
 * a deterministic sum.
 *
 * c_buf_alloc / c_buf_free let the test alloc through the C side
 * (instead of ffi_alloc), exercising "pointer returned from C is
 * valid when passed back to C".
 * ============================================================ */

CFFI_EXPORT void c_buf_fill(void *p, int n, int byte_value) {
  memset(p, byte_value & 0xFF, n);
}

CFFI_EXPORT uint32_t c_buf_checksum(const void *p, int n) {
  const unsigned char *b = (const unsigned char *)p;
  uint32_t sum = 0;
  for (int i = 0; i < n; i++) {
    sum = (sum * 31u) + b[i];
  }
  return sum;
}

CFFI_EXPORT void *c_buf_alloc(int n) {
  return malloc(n);
}

CFFI_EXPORT void c_buf_free(void *p) {
  free(p);
}

/* Write a known 32-bit pattern at byte offset; tests that the
 * caller can read it back through ffi_get uint32_t. */
CFFI_EXPORT void c_buf_set_u32(void *p, int ofs, uint32_t v) {
  memcpy((unsigned char *)p + ofs, &v, sizeof(uint32_t));
}
CFFI_EXPORT uint32_t c_buf_get_u32(const void *p, int ofs) {
  uint32_t v;
  memcpy(&v, (const unsigned char *)p + ofs, sizeof(uint32_t));
  return v;
}

/* Copy n bytes from src to dst, return dst. Exercises three-ptr
 * arg passing. */
CFFI_EXPORT void *c_buf_copy(void *dst, const void *src, int n) {
  memcpy(dst, src, n);
  return dst;
}


/* ============================================================
 * Float edge cases
 *
 * Verifies that special float bit patterns (NaN, Inf, denormal,
 * negative zero) survive the call. Some FFI bugs swap the high
 * and low halves of a double, which mangles only certain bit
 * patterns.
 * ============================================================ */

CFFI_EXPORT double c_f64_neg(double x)   { return -x; }
CFFI_EXPORT float  c_f32_neg(float x)    { return -x; }

/* Returns x as a sequence of operations that exercise the float
 * pipeline. If any stage drops bits, the result diverges. */
CFFI_EXPORT double c_f64_seq(double x) {
  double y = x * 2.0;
  y = y + 1.0;
  y = y / 3.0;
  return y;
}


/* ============================================================
 * Pointer roundtrip with multiple steps
 *
 * Allocates a buffer, passes through several ptr-typed FFI
 * boundaries, then asks for its content. Catches lifetime bugs
 * where Symta's pointer-to-fixnum encoding loses bits.
 * ============================================================ */

CFFI_EXPORT void *c_ptr_inc(void *p) {
  return (unsigned char *)p + 1;
}
CFFI_EXPORT int c_ptr_diff(const void *a, const void *b) {
  return (int)((const unsigned char *)a - (const unsigned char *)b);
}


/* ============================================================
 * Stress: hash a callback-style int sequence.
 *
 * Symta calls this in a tight loop with sequential ints. The
 * function returns a deterministic hash of the input, so the
 * Symta side can verify "all N calls returned the right value".
 * Catches per-call state leakage (e.g. an arg register left
 * uncleared from a previous call leaks into this one's args).
 * ============================================================ */

CFFI_EXPORT uint32_t c_hash_i32(int x) {
  uint32_t v = (uint32_t)x;
  v ^= v >> 16;
  v *= 0x7feb352du;
  v ^= v >> 15;
  v *= 0x846ca68bu;
  v ^= v >> 16;
  return v;
}


/* ============================================================
 * Library version probe — so the test can print which build of
 * cffilib it's talking to. Useful for "did we pick up the new
 * library or the cached old one?" debugging.
 * ============================================================ */

CFFI_EXPORT const char *cffi_version(void) {
  return "cffilib-1.0";
}
