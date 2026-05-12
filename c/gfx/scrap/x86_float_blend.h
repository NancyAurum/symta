__m128 srgb = _mm_cvtepi32_ps(_mm_setr_epi32(sb,sg,sr,0));
__m128 drgb = _mm_cvtepi32_ps(_mm_shuffle_epi8(_mm_set1_epi32(DC),imask));
__m128 msa = _mm_set1_ps((float)sa*(1.0f/255.0f));
__m128 r1 = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(drgb,srgb), msa), srgb);
__m128 r2 = _mm_cvtps_epi32(r1);
DC = _mm_extract_epi32(_mm_shuffle_epi8(r2,omask),0);
//int32_t o[4];
//_mm_storeu_si128((__m128i*)o, _mm_shuffle_epi8(r2,omask));
//DC = o[0];