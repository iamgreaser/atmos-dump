/*
x86_64 has SSE2 as a minimum requirement
- this flag is useful for testing the reference implementation
  on this platform!
*/

#ifdef NO_SIMD
#undef __MMX_
#undef __SSE__
#undef __SSE2__
#undef __SSE3__
#undef __SSSE3__
#undef __SSE4_2__
#undef __SSE4_1__
#undef __SSE4A__
#undef __SSE4B__
#undef __AVX__
#undef __AES__
#undef __PCLMUL__
#undef __RDRND__
#undef __F16C__
#endif

#include <immintrin.h>

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#include <sys/time.h>

/* we'll probably need this :) --GM */
#include <math.h>

/* some help wrt intrinsic handling */
/* NOTE: AVX routines not implemented yet! */
#ifdef __AVX__
typedef __m256 msimdf;
#else
#ifdef __SSE__
typedef __m128 msimdf;
#endif
#endif

typedef enum tile_ee
{
	TILE_SPACE = 0,
	TILE_WALL,
	TILE_FLOOR,
	TILE_DOOR,
	
	TILE_MAX
} tile_e;

typedef struct tile
{
	// basic data
	uint8_t type;
	uint8_t param;
	
	// pointers
} tile_t;

typedef struct map
{
	/* width MUST be divisible by 8 - this is for SSE/AVX! */
	int width, height;
	
	/* former is aligned, latter is what you free() up */
	float *outpres, *alloc_outpres;
	float *flow,    *alloc_flow;
	float *dirbufx,    *alloc_dirbufx;
	float *dirbufy,    *alloc_dirbufy;
	float *inpres_o2,  *alloc_inpres_o2;
	float *inpres_air,  *alloc_inpres_air;
	
	/* the rest doesn't need to be alligned */
	tile_t *tile;
} map_t;

/* atmos.c */
map_t *map_create(void);

/* flow.c */
void update_pres(map_t *map, float *inpres, float *outpres, float *flow);


