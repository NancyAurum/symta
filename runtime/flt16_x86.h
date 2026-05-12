//#include <emmintrin.h>
#include <smmintrin.h>

//_MM_FROUND_TO_NEAREST_INT, _MM_FROUND_TO_ZERO,_MM_FROUND_TO_POS_INF
//_MM_FROUND_TO_NEG_INF, _MM_FROUND_CUR_DIRECTION

//C/C++ defaults to _MM_FROUND_TO_ZERO, but _MM_FROUND_CUR_DIRECTION is faster
//becasue it shouldn't emit mode change opcode
#define HF_ROUND_MODE _MM_FROUND_CUR_DIRECTION

__attribute__ ((__target__("f16c"))) uint16_t x86_f2h(float value) {
  return _mm_cvtsi128_si32(_mm_cvtps_ph(_mm_set_ss(value), HF_ROUND_MODE));
}

__attribute__ ((__target__("f16c"))) float x86_h2f(uint16_t value) {
  return _mm_cvtss_f32(_mm_cvtph_ps(_mm_cvtsi32_si128(value)));
}
