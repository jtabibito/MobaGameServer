#ifndef __CACHE_ALLOC_H__
#define __CACHE_ALLOC_H__

#ifdef __cplusplus
extern "C" {
#endif
	struct cache_allocer* CreateCacheAllocer(int capacity, int elementSize);
	void DestroyCacheAllocer(struct cache_allocer* allocer);

	void* CacheAlloc(struct cache_allocer* allocer, int elementSize);
	void CacheFree(struct cache_allocer* allocer, void* mem);

#ifdef __cplusplus
}
#endif
#endif // !__CACHE_ALLOC_H__
