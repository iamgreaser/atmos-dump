/*
x86_64 requires SSE2.
also, x87 FPU is 80-bit FP while SSE is 32-bit or (since SSE2) 64-bit.
we want to test using SSE2 code, but w/o intrinsics, for our test cases.

it also means we can use the FPU registers for MMX instead :)
but i think we'll just stick with the SSE2-provided XMM integer ops;
they should be faster.

having said that, non-SSE2 code can be tested by compiling for 32-bit.

having said THAT, 64-bit mode gives you 16 XMM registers as opposed to 8 :)

oh yeah, don't forget, compile with -O2! otherwise SSE will suck horribly
seriously, compare a -O2 disassembly with a non--O2 disassembly;
hopefully you'll see that the latter does a crapton more memory accesses
instead of compacting stuff down nicely into XMM registers.

...and after typing all that: i'm a guy. --GM

according to genderanalyzer.com:
"We have strong indicators that [url used] is written by a man (95%)."
*phew*.

*/

/*
WARNING:
As it stands, the SSE and AVX versions give SLIGHTLY different results
from the reference implementation. Proceed with caution.

*/

/*
useful flags:
-DNO_SIMD
	test w/o using SIMD opcodes
*/

/*
 * oh yeah, prefetches.
 *   these are a little bit of theory,
 *     and a lot of trial and error.
 *
 * performance may actually be terrible on your machine.
 *
 * however, it's quite fantastic on this one!
 * using a 512x512 map to test w/ 1000 iterations:
 *
 * (new ref: profile 1856/3654717 ticks)
 * ref: profile 2168/3873547 ticks (1.000x)
 * SSE: profile  909/1800992 ticks (2.151x)
 * AVX: profile  963/1798748 ticks (2.153x)
 *
 * of course, there aren't many CPUs with AVX, heh
 */
 
#include "common.h"

#ifdef __SSE__
#ifdef __AVX__
#pragma message "Using AVX optimisations"
#else
#pragma message "Using SSE optimisations"
#endif
#else
#pragma message "Using reference implementation"
#endif

int profiler_ticks = 0;

void update_pres(map_t *map, float *inpres, float *outpres, float *flow)
{
	int width = map->width;
	int height = map->height;
	
	int x,y;
	
	uint64_t time_start;
	uint64_t time_end;
	{
		struct timeval tv;
		gettimeofday(&tv, NULL);
		time_start = tv.tv_sec*1000000+tv.tv_usec;
	}
	float *dirbufx = map->dirbufx;
	float *dirbufy = map->dirbufy;
	
	/* NOTE: SHOULD ASSERT WIDTH DIVISIBLE BY 4! */
	/* NOTE: SHOULD ASSERT MAP.INPRES/OUTPRES/FLOW 16-BYTE ALIGNED! */
	int idx = 0;
	

#ifdef __SSE__
	static const float fconst_1 = 1.0f;
	static const float fconst_1o4 = 1.0f/4.0f;
#ifdef __AVX__
	__m256 xmm_upres = _mm256_setzero_ps();
	__m256 xmm_uflow = _mm256_broadcast_ss(&fconst_1);
	__m256 xmm_dpres;
	__m256 xmm_dflow;
	const __m256 xmmconst_all1o4 = _mm256_broadcast_ss(&fconst_1o4);
	const __m256 xmmconst_all1 = _mm256_broadcast_ss(&fconst_1);
	const __m256 xmmconst_all0 = _mm256_setzero_ps();
#else
	__m128 xmm_upres = _mm_setzero_ps();
	__m128 xmm_uflow = _mm_set1_ps(1.0f);
	__m128 xmm_dpres;
	__m128 xmm_dflow;
	const __m128 xmmconst_all1o4 = _mm_set1_ps(1.0f/4.0f);
	const __m128 xmmconst_all1 = _mm_set1_ps(1.0f);
	const __m128 xmmconst_all0 = _mm_setzero_ps();
#endif
#endif
	
	for(y = 0; y < height; y++)
	{
#ifdef __SSE__
		_mm_prefetch(&inpres[idx], _MM_HINT_T0);
		
#ifdef __AVX__
		__m256 xmm_xpres = _mm256_setzero_ps();
		__m256 xmm_xflow = _mm256_broadcast_ss(&fconst_1);
#else
		__m128 xmm_xpres = _mm_setzero_ps();
		__m128 xmm_xflow = _mm_set1_ps(1.0f);
#endif
		
		if(y >= height-1)
		{
#ifdef __AVX__
			xmm_dpres = _mm256_setzero_ps();
			xmm_dflow = xmmconst_all1;
#else
			xmm_dpres = _mm_setzero_ps();
			xmm_dflow = xmmconst_all1;
#endif
		}
		_mm_prefetch(&flow[idx], _MM_HINT_T0);
#else
		float lpres = 0.0f;
		float lflow = 1.0f;
		float xpres = inpres[idx];
		float xflow = flow[idx];
#endif
#ifdef __SSE__
#ifdef __AVX__
#define XINCSIZE 8
#else
#define XINCSIZE 4
#endif
		for(x = 0; x < width; x+=XINCSIZE,idx+=XINCSIZE)
		{
			/* i had a really clever SSE thing before
			 * but performancewise it absolutely sucked
			 *
			 * what the hell is wrong with you, SSE???
			 * you're supposed to ALWAYS be good
			 * not VERY RARELY good and ALMOST ALWAYS crap
			 *
			 * anyhow, load everything we need for this part
			 */
#ifdef __AVX__
			__m256 xmm_newpres = _mm256_load_ps(&inpres[idx]);
			__m256 xmm_newflow = _mm256_load_ps(&flow[idx]);
			if(y > 0)
			{
				xmm_upres = _mm256_load_ps(&inpres[idx-width]);
				xmm_uflow = _mm256_load_ps(&flow[idx-width]);
			}
			if(y < height-1)
			{
				xmm_dpres = _mm256_load_ps(&inpres[idx+width]);
				xmm_dflow = _mm256_load_ps(&flow[idx+width]);
			}
#else
			__m128 xmm_newpres = _mm_load_ps(&inpres[idx]);
			__m128 xmm_newflow = _mm_load_ps(&flow[idx]);
			if(y > 0)
			{
				xmm_upres = _mm_load_ps(&inpres[idx-width]);
				xmm_uflow = _mm_load_ps(&flow[idx-width]);
			}
			if(y < height-1)
			{
				xmm_dpres = _mm_load_ps(&inpres[idx+width]);
				xmm_dflow = _mm_load_ps(&flow[idx+width]);
			}
#endif

#if 0
#ifdef __AVX__
			float rpres = x < width-8 ? inpres[idx+8] : 0.0f;
			float rflow = x < width-8 ? flow[idx+8] : 1.0f;
#else
			float rpres = x < width-4 ? inpres[idx+4] : 0.0f;
			float rflow = x < width-4 ? flow[idx+4] : 1.0f;
#endif
			
#ifdef __AVX__
			/* this is a part where AVX is *kinda* crap.
			 * it helps to think of AVX as "2x speed SSE"
			 * rather than "double-sized SSE".
			 *
			 * for this we're going to use proper rotates
			 * so we don't lose data.
			 *
			 * L: 0123|4567 -> 3012|7456 -> 3012|3456 -> *012|3456
			 * R: 0123|4567 -> 1230|5674 -> 1234|5674 -> 1234|567*
			 */
			__m256 xmm_lpres = _mm256_shuffle_ps(xmm_newpres, xmm_newpres, 0x93);
			__m256 xmm_lflow = _mm256_shuffle_ps(xmm_newflow, xmm_newflow, 0x93);
			__m256 xmm_rpres = _mm256_shuffle_ps(xmm_newpres, xmm_newpres, 0x39);
			__m256 xmm_rflow = _mm256_shuffle_ps(xmm_newflow, xmm_newflow, 0x39);
			/* we have to manually copy the middle value across. lovely. */
			xmm_lpres[4] = xmm_lpres[0];
			xmm_lflow[4] = xmm_lflow[0];
			xmm_rpres[3] = xmm_rpres[7];
			xmm_rflow[3] = xmm_rflow[7];
			/* back to the usual. */
			xmm_lpres[0] = xmm_xpres[7];
			xmm_lflow[0] = xmm_xflow[7];
			xmm_rpres[7] = rpres;
			xmm_rflow[7] = rflow;
#else
			__m128 xmm_lpres = _mm_shuffle_ps(xmm_newpres, xmm_newpres, 0x93);
			__m128 xmm_lflow = _mm_shuffle_ps(xmm_newflow, xmm_newflow, 0x93);
			__m128 xmm_rpres = _mm_shuffle_ps(xmm_newpres, xmm_newpres, 0x39);
			__m128 xmm_rflow = _mm_shuffle_ps(xmm_newflow, xmm_newflow, 0x39);
			xmm_lpres[0] = xmm_xpres[3];
			xmm_lflow[0] = xmm_xflow[3];
			xmm_rpres[3] = rpres;
			xmm_rflow[3] = rflow;
#endif
#else
#ifdef __AVX__
			
			__m256 xmm_lpres = _mm256_loadu_ps(&inpres[idx-1]);
			__m256 xmm_lflow = _mm256_loadu_ps(&flow[idx-1]);
			__m256 xmm_rpres = _mm256_loadu_ps(&inpres[idx+1]);
			__m256 xmm_rflow = _mm256_loadu_ps(&flow[idx+1]);
			
			if(x == 0)
			{
				xmm_lpres[0] = 0.0f;
				xmm_lflow[0] = 1.0f;
			} else if(x >= width-8) {
				xmm_rpres[7] = 0.0f;
				xmm_rflow[7] = 1.0f;
			}
#else
			__m128 xmm_lpres = _mm_loadu_ps(&inpres[idx-1]);
			__m128 xmm_lflow = _mm_loadu_ps(&flow[idx-1]);
			__m128 xmm_rpres = _mm_loadu_ps(&inpres[idx+1]);
			__m128 xmm_rflow = _mm_loadu_ps(&flow[idx+1]);
			if(x == 0)
			{
				xmm_lpres[0] = 0.0f;
				xmm_lflow[0] = 1.0f;
			} else if(x >= width-4) {
				xmm_rpres[3] = 0.0f;
				xmm_rflow[3] = 1.0f;
			}
#endif
#endif
			
			/* PREFETCH */
			_mm_prefetch(&inpres[idx+64], _MM_HINT_T0);
			
			xmm_xpres = xmm_newpres;
			xmm_xflow = xmm_newflow;
			
			/* now do some calculations
			 * start with the subtraction
			 */
#ifdef __AVX__
			xmm_lpres = _mm256_sub_ps(xmm_lpres, xmm_xpres);
			xmm_rpres = _mm256_sub_ps(xmm_rpres, xmm_xpres);
			xmm_upres = _mm256_sub_ps(xmm_upres, xmm_xpres);
			xmm_dpres = _mm256_sub_ps(xmm_dpres, xmm_xpres);
#else
			xmm_lpres = _mm_sub_ps(xmm_lpres, xmm_xpres);
			xmm_rpres = _mm_sub_ps(xmm_rpres, xmm_xpres);
			xmm_upres = _mm_sub_ps(xmm_upres, xmm_xpres);
			xmm_dpres = _mm_sub_ps(xmm_dpres, xmm_xpres);
#endif

			/* PREFETCH */
			_mm_prefetch(&flow[idx+64], _MM_HINT_T0);
				
			
			/* use this gap to divide xflow by 4 */
#ifdef __AVX__
			__m256 xmm_xflowd4 = _mm256_mul_ps(xmm_xflow, xmmconst_all1o4);
#else
			__m128 xmm_xflowd4 = _mm_mul_ps(xmm_xflow, xmmconst_all1o4);
#endif			
			_mm_prefetch(&dirbufx[idx], _MM_HINT_T0);
			/* then the multiplication */
#ifdef __AVX__
			xmm_lpres = _mm256_mul_ps(xmm_lpres, xmm_lflow);
			xmm_rpres = _mm256_mul_ps(xmm_rpres, xmm_rflow);
			xmm_upres = _mm256_mul_ps(xmm_upres, xmm_uflow);
			xmm_dpres = _mm256_mul_ps(xmm_dpres, xmm_dflow);
#else
			xmm_lpres = _mm_mul_ps(xmm_lpres, xmm_lflow);
			xmm_rpres = _mm_mul_ps(xmm_rpres, xmm_rflow);
			xmm_upres = _mm_mul_ps(xmm_upres, xmm_uflow);
			xmm_dpres = _mm_mul_ps(xmm_dpres, xmm_dflow);
#endif
			/* PREFETCH */
			_mm_prefetch(&inpres[idx+width+64], _MM_HINT_T0);

			/* and now we perform the divide-o-rama */
#ifdef __AVX__
			xmm_lpres = _mm256_mul_ps(xmm_lpres, xmm_xflowd4);
			xmm_rpres = _mm256_mul_ps(xmm_rpres, xmm_xflowd4);
			xmm_upres = _mm256_mul_ps(xmm_upres, xmm_xflowd4);
			xmm_dpres = _mm256_mul_ps(xmm_dpres, xmm_xflowd4);
#else
			xmm_lpres = _mm_mul_ps(xmm_lpres, xmm_xflowd4);
			xmm_rpres = _mm_mul_ps(xmm_rpres, xmm_xflowd4);
			xmm_upres = _mm_mul_ps(xmm_upres, xmm_xflowd4);
			xmm_dpres = _mm_mul_ps(xmm_dpres, xmm_xflowd4);
#endif
			
			/* use this gap to load the dirbufs */
#ifdef __AVX__
			__m256 xmm_dirbufx = _mm256_load_ps(&dirbufx[idx]);
			__m256 xmm_dirbufy = _mm256_load_ps(&dirbufy[idx]);
#else
			__m128 xmm_dirbufx = _mm_load_ps(&dirbufx[idx]);
			__m128 xmm_dirbufy = _mm_load_ps(&dirbufy[idx]);
#endif
			
			/* add the pairs together */
#ifdef __AVX__
			__m256 xmm_hpres = _mm256_add_ps(xmm_lpres, xmm_rpres);
			__m256 xmm_vpres = _mm256_add_ps(xmm_upres, xmm_dpres);
#else
			__m128 xmm_hpres = _mm_add_ps(xmm_lpres, xmm_rpres);
			__m128 xmm_vpres = _mm_add_ps(xmm_upres, xmm_dpres);
#endif
			
			/* PREFETCH */
			_mm_prefetch(&flow[idx+width+64], _MM_HINT_T0);
			
			/* add them to the dirbuf */
#ifdef __AVX__
			xmm_dirbufx = _mm256_add_ps(xmm_dirbufx, xmm_hpres);
			xmm_dirbufy = _mm256_add_ps(xmm_dirbufy, xmm_vpres);
#else
			xmm_dirbufx = _mm_add_ps(xmm_dirbufx, xmm_hpres);
			xmm_dirbufy = _mm_add_ps(xmm_dirbufy, xmm_vpres);
#endif
			/* add the remaining pair together */
			/* and add them to xpres */
			_mm_prefetch(&dirbufy[idx+64], _MM_HINT_T0);
#ifdef __AVX__
			__m256 xmm_cpres = _mm256_add_ps(xmm_hpres, xmm_vpres);
			xmm_xpres = _mm256_add_ps(xmm_xpres, xmm_cpres);
#else
			__m128 xmm_cpres = _mm_add_ps(xmm_hpres, xmm_vpres);
			xmm_xpres = _mm_add_ps(xmm_xpres, xmm_cpres);
#endif
			
			/* store it all */
#ifdef __AVX__
			_mm256_store_ps(&dirbufx[idx], xmm_dirbufx);
			_mm256_store_ps(&dirbufy[idx], xmm_dirbufy);
			_mm256_store_ps(&outpres[idx], xmm_xpres);
#else
			_mm_store_ps(&dirbufx[idx], xmm_dirbufx);
			_mm_store_ps(&dirbufy[idx], xmm_dirbufy);
			_mm_store_ps(&outpres[idx], xmm_xpres);
#endif
		}
#else
		for(x = 0; x < width; x++,idx++)
		{
			float rpres = (x >= width-1 ? 0.0 : inpres[idx+1]);
			float rflow = (x >= width-1 ? 1.0 : flow[idx+1]);
			float upres = (y <= 0 ? 0.0 : inpres[idx-width]);
			float uflow = (y <= 0 ? 1.0 : flow[idx-width]);
			float dpres = (y >= height-1 ? 0.0 : inpres[idx+width]);
			float dflow = (y >= height-1 ? 1.0 : flow[idx+width]);

			float ldelta = ((lpres-xpres)*lflow);
			float rdelta = ((rpres-xpres)*rflow);
			float udelta = ((upres-xpres)*uflow);
			float ddelta = ((dpres-xpres)*dflow);
			
			float hdelta = (ldelta+rdelta)*xflow;
			float vdelta = (udelta+ddelta)*xflow;
			
			dirbufx[idx] += hdelta;
			dirbufy[idx] += vdelta;
			outpres[idx] = xpres + (hdelta + vdelta)/4.0f;
			
			lpres = xpres;
			lflow = xflow;
			xpres = rpres;
			xflow = rflow;
		}
#endif
	}
	
	{
		struct timeval tv;
		gettimeofday(&tv, NULL);
		time_end = tv.tv_sec*1000000+tv.tv_usec;
	}
	
	//printf("profile %i ticks\n", time_end - time_start);
	int tdiff = time_end - time_start;
	profiler_ticks += tdiff;
	printf("profile %i/%i ticks\n", tdiff, profiler_ticks);
}

