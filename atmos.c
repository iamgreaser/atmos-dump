/*
atmospherics simulation
*/

#include "common.h"

void alloc_aligned(void **palign, void **palloc, int size, int alignment)
{
	void *v = malloc(size+alignment);
	
	if(v == NULL)
	{
		fprintf(stderr, "FATAL: Could not allocate %i bytes %i-aligned\n"
			, size, alignment);
		*palign = *palloc = NULL;
		return;
	}
	
	*palloc = v;
	*palign = (v+alignment-1) - (((long)v+alignment-1) % alignment);
}

void swap_pfloat(float **a, float **b)
{
	float *t = *a;
	*a = *b;
	*b = t;
}

void map_clear(map_t *map)
{
	int x,y;
	
	for(y = 0; y < map->height; y++)
	for(x = 0; x < map->width; x++)
	{
		int idx = y*map->width+x;
		map->tile[idx].type = TILE_SPACE;
		map->tile[idx].param = 0;
		//map->inpres_o2[idx] = 0.0f;
		//map->inpres_air[idx] = 0.0f;
		// TEST: random values for pressures
		map->inpres_o2[idx] = (rand() % 373) * 2.0f / 372.0f;
		map->inpres_air[idx] = (rand() % 373) * 2.0f / 372.0f;
		map->outpres[idx] = 0.0f;
		map->flow[idx] = 1.0f;
		map->dirbufx[idx] = 0.0f;
		map->dirbufy[idx] = 0.0f;
	}
}

void map_clear_dirbuf(map_t *map)
{
	int x,y;
	
	for(y = 0; y < map->height; y++)
	for(x = 0; x < map->width; x++)
	{
		int idx = y*map->width+x;
		map->dirbufx[idx] = 0.0f;
		map->dirbufy[idx] = 0.0f;
	}
}

map_t *map_new(int width, int height)
{
	if(width & 7)
	{
		fprintf(stderr, "ASSERTION: Width must be divisible by 8!\n");
		return NULL;
	}
	
	map_t *map = malloc(sizeof(map_t));
	if(map == NULL)
	{
		fprintf(stderr, "FATAL: Could not allocate map!\n");
		return NULL;
	}
	
	map->width = width;
	map->height = height;
	
	alloc_aligned((void **)&map->flow, (void **)&map->alloc_flow, 4*width*height, 32);
	alloc_aligned((void **)&map->outpres, (void **)&map->alloc_outpres, 4*width*height, 32);
	alloc_aligned((void **)&map->inpres_o2, (void **)&map->alloc_inpres_o2, 4*width*height, 32);
	alloc_aligned((void **)&map->inpres_air, (void **)&map->alloc_inpres_air, 4*width*height, 32);
	alloc_aligned((void **)&map->dirbufx, (void **)&map->alloc_dirbufx, 4*width*height, 32);
	alloc_aligned((void **)&map->dirbufy, (void **)&map->alloc_dirbufy, 4*width*height, 32);
	map->tile = malloc(width*height*sizeof(tile_t));
	
	map_clear(map);
	
	return map;
}

void map_free(map_t *map)
{
	free(map->alloc_outpres);
	free(map->alloc_flow);
	free(map->alloc_inpres_air);
	free(map->alloc_inpres_o2);
	free(map->tile);
	free(map);
}

void atmos_showpres(map_t *map)
{
	int x,y;
	
	for(y=0;y<map->height;y++)
	{
		for(x=0;x<map->width;x++)
		{
			int idx = y*map->width+x;
			printf("%c", 'A'+(int)(0.5f+4.0f*map->inpres_o2[idx]));
			//printf("%c", 'A'+(int)(0.5f+8.0f*map->dirbufx[idx]));
		}
		printf("\n");
	}
	printf("\n");
}

void atmos_tick(map_t *map)
{
#ifdef __AVX__
	_mm256_zeroall();
#endif
	
	/* NOTE: o2 is breathable; air is NOT!
	 */
	map_clear_dirbuf(map);
	
	/* for slightly better cache performance
	 * we could make it update all pressures
	 * but this is a bit simpler to do
	 * on top of that, i think the cache performance is pretty good as it is
	 */
	update_pres(map, map->inpres_o2, map->outpres, map->flow);
	swap_pfloat(&map->inpres_o2, &map->outpres);
	swap_pfloat(&map->alloc_inpres_o2, &map->alloc_outpres);
	
	update_pres(map, map->inpres_air, map->outpres, map->flow);
	swap_pfloat(&map->inpres_air, &map->outpres);
	swap_pfloat(&map->alloc_inpres_air, &map->alloc_outpres);
}

int main(int argc, char *argv[])
{
	// disable denormals
#ifdef __SSE__
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
	_MM_SET_EXCEPTION_MASK(0xFFFF);
#else
	// TODO: x87 variant
#endif
	
	int i;
	//map_t *map = map_new(64,20);
	map_t *map = map_new(512,512);
	
	//atmos_showpres(map);
	for(i=0; i<1000; i++)
	{
		atmos_tick(map);
		//atmos_showpres(map);
		//usleep(10000);
	}
	//atmos_showpres(map);
	
	map_free(map);
	
	return 0;
}

