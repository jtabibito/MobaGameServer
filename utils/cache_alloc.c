#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cache_alloc.h"

struct node {
	struct node* next;
};

struct cache_allocer {
	int capacity;	//容量
	int elementSize;
	unsigned char* cache_mem;	// 分配的内存
	struct node* free_list;	// 可用列表头
};

struct cache_allocer*
CreateCacheAllocer(int capacity, int elementSize) {
	struct cache_allocer* allocer = malloc(sizeof(struct cache_allocer));
	memset(allocer, 0, sizeof(struct cache_allocer));

	// 保证每个节点能存下一个指针
	elementSize = (elementSize < sizeof(struct node) ? sizeof(struct node) : elementSize);
	allocer->capacity = capacity;
	allocer->elementSize = elementSize;
	allocer->cache_mem = malloc(capacity * elementSize);
	memset(allocer->cache_mem, 0, sizeof(capacity * elementSize));

	allocer->free_list = NULL;
	for (int i = 0; i < capacity; i++) {
		struct node* transer = (struct node*)(allocer->cache_mem + i * elementSize);
		transer->next = allocer->free_list;
		allocer->free_list = transer;
	}

	return allocer;
}

// 通常不删除内存池
void
DestroyCacheAllocer(struct cache_allocer* allocer) {
	if (allocer->cache_mem != NULL) {
		free(allocer->cache_mem);
	}

	free(allocer);
}

void*
CacheAlloc(struct cache_allocer* allocer, int elementSize) {
	if (allocer->elementSize < elementSize) {
		return malloc(elementSize);
	}

	if (allocer->free_list != NULL) {
		void* current = allocer->free_list;
		allocer->free_list = allocer->free_list->next;
		return current;
	}

	return malloc(elementSize);
}

void
CacheFree(struct cache_allocer* allocer, void* mem) {
	if (((unsigned char*)mem) >= allocer->cache_mem &&
		((unsigned char*)mem) < allocer->cache_mem + allocer->capacity * allocer->elementSize) {
		struct node* node = mem;
		node->next = allocer->free_list;
		allocer->free_list = node;
		return;
	}

	free(mem);
}