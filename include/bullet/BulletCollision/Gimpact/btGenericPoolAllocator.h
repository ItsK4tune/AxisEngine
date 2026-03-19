


#ifndef BT_GENERIC_POOL_ALLOCATOR_H
#define BT_GENERIC_POOL_ALLOCATOR_H

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "LinearMath/btAlignedAllocator.h"

#define BT_UINT_MAX UINT_MAX
#define BT_DEFAULT_MAX_POOLS 16


class btGenericMemoryPool
{
public:
	unsigned char *m_pool;      
	size_t *m_free_nodes;       
	size_t *m_allocated_sizes;  
	size_t m_allocated_count;
	size_t m_free_nodes_count;

protected:
	size_t m_element_size;
	size_t m_max_element_count;

	size_t allocate_from_free_nodes(size_t num_elements);
	size_t allocate_from_pool(size_t num_elements);

public:
	void init_pool(size_t element_size, size_t element_count);

	void end_pool();

	btGenericMemoryPool(size_t element_size, size_t element_count)
	{
		init_pool(element_size, element_count);
	}

	~btGenericMemoryPool()
	{
		end_pool();
	}

	inline size_t get_pool_capacity()
	{
		return m_element_size * m_max_element_count;
	}

	inline size_t gem_element_size()
	{
		return m_element_size;
	}

	inline size_t get_max_element_count()
	{
		return m_max_element_count;
	}

	inline size_t get_allocated_count()
	{
		return m_allocated_count;
	}

	inline size_t get_free_positions_count()
	{
		return m_free_nodes_count;
	}

	inline void *get_element_data(size_t element_index)
	{
		return &m_pool[element_index * m_element_size];
	}

	
	
	void *allocate(size_t size_bytes);

	bool freeMemory(void *pointer);
};



class btGenericPoolAllocator
{
protected:
	size_t m_pool_element_size;
	size_t m_pool_element_count;

public:
	btGenericMemoryPool *m_pools[BT_DEFAULT_MAX_POOLS];
	size_t m_pool_count;

	inline size_t get_pool_capacity()
	{
		return m_pool_element_size * m_pool_element_count;
	}

protected:
	
	btGenericMemoryPool *push_new_pool();

	void *failback_alloc(size_t size_bytes);

	bool failback_free(void *pointer);

public:
	btGenericPoolAllocator(size_t pool_element_size, size_t pool_element_count)
	{
		m_pool_count = 0;
		m_pool_element_size = pool_element_size;
		m_pool_element_count = pool_element_count;
	}

	virtual ~btGenericPoolAllocator();

	
	
	void *allocate(size_t size_bytes);

	bool freeMemory(void *pointer);
};

void *btPoolAlloc(size_t size);
void *btPoolRealloc(void *ptr, size_t oldsize, size_t newsize);
void btPoolFree(void *ptr);

#endif
