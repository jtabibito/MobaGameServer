#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cache_alloc.h"
#include "small_alloc.h"

#define SMALL_ELEM_NUM 10*1024
#define SMALL_ELEM_SIZE 128

static struct cache_allocer* small_allocer = NULL;

void*
small_alloc(int size) {
	if (small_allocer == NULL) {
		small_allocer = CreateCacheAllocer(SMALL_ELEM_NUM, SMALL_ELEM_SIZE);
	}
	return CacheAlloc(small_allocer, size);
}

void
small_free(void* mem) {
	if (small_allocer == NULL) {
		small_allocer = CreateCacheAllocer(SMALL_ELEM_NUM, SMALL_ELEM_SIZE);
	}
	CacheFree(small_allocer, mem);
}
