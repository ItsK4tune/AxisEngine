


#include "btGenericPoolAllocator.h"



size_t btGenericMemoryPool::allocate_from_free_nodes(size_t num_elements)
{
	size_t ptr = BT_UINT_MAX;

	if (m_free_nodes_count == 0) return BT_UINT_MAX;
	
	size_t revindex = m_free_nodes_count;

	while (revindex-- && ptr == BT_UINT_MAX)
	{
		if (m_allocated_sizes[m_free_nodes[revindex]] >= num_elements)
		{
			ptr = revindex;
		}
	}
	if (ptr == BT_UINT_MAX) return BT_UINT_MAX;  

	revindex = ptr;
	ptr = m_free_nodes[revindex];
	

	size_t finalsize = m_allocated_sizes[ptr];
	finalsize -= num_elements;

	m_allocated_sizes[ptr] = num_elements;

	

	if (finalsize > 0)  
	{
		m_free_nodes[revindex] = ptr + num_elements;
		m_allocated_sizes[ptr + num_elements] = finalsize;
	}
	else  
	{
		
		m_free_nodes[revindex] = m_free_nodes[m_free_nodes_count - 1];
		m_free_nodes_count--;
	}

	return ptr;
}

size_t btGenericMemoryPool::allocate_from_pool(size_t num_elements)
{
	if (m_allocated_count + num_elements > m_max_element_count) return BT_UINT_MAX;

	size_t ptr = m_allocated_count;

	m_allocated_sizes[m_allocated_count] = num_elements;
	m_allocated_count += num_elements;

	return ptr;
}

void btGenericMemoryPool::init_pool(size_t element_size, size_t element_count)
{
	m_allocated_count = 0;
	m_free_nodes_count = 0;

	m_element_size = element_size;
	m_max_element_count = element_count;

	m_pool = (unsigned char *)btAlignedAlloc(m_element_size * m_max_element_count, 16);
	m_free_nodes = (size_t *)btAlignedAlloc(sizeof(size_t) * m_max_element_count, 16);
	m_allocated_sizes = (size_t *)btAlignedAlloc(sizeof(size_t) * m_max_element_count, 16);

	for (size_t i = 0; i < m_max_element_count; i++)
	{
		m_allocated_sizes[i] = 0;
	}
}

void btGenericMemoryPool::end_pool()
{
	btAlignedFree(m_pool);
	btAlignedFree(m_free_nodes);
	btAlignedFree(m_allocated_sizes);
	m_allocated_count = 0;
	m_free_nodes_count = 0;
}



void *btGenericMemoryPool::allocate(size_t size_bytes)
{
	size_t module = size_bytes % m_element_size;
	size_t element_count = size_bytes / m_element_size;
	if (module > 0) element_count++;

	size_t alloc_pos = allocate_from_free_nodes(element_count);
	
	if (alloc_pos != BT_UINT_MAX)
	{
		return get_element_data(alloc_pos);
	}
	
	alloc_pos = allocate_from_pool(element_count);

	if (alloc_pos == BT_UINT_MAX) return NULL;  
	return get_element_data(alloc_pos);
}

bool btGenericMemoryPool::freeMemory(void *pointer)
{
	unsigned char *pointer_pos = (unsigned char *)pointer;
	unsigned char *pool_pos = (unsigned char *)m_pool;
	
	if (pointer_pos < pool_pos) return false;  
	size_t offset = size_t(pointer_pos - pool_pos);
	if (offset >= get_pool_capacity()) return false;  

	
	m_free_nodes[m_free_nodes_count] = offset / m_element_size;
	m_free_nodes_count++;
	return true;
}



btGenericPoolAllocator::~btGenericPoolAllocator()
{
	
	size_t i;
	for (i = 0; i < m_pool_count; i++)
	{
		m_pools[i]->end_pool();
		btAlignedFree(m_pools[i]);
	}
}


btGenericMemoryPool *btGenericPoolAllocator::push_new_pool()
{
	if (m_pool_count >= BT_DEFAULT_MAX_POOLS) return NULL;

	btGenericMemoryPool *newptr = (btGenericMemoryPool *)btAlignedAlloc(sizeof(btGenericMemoryPool), 16);

	m_pools[m_pool_count] = newptr;

	m_pools[m_pool_count]->init_pool(m_pool_element_size, m_pool_element_count);

	m_pool_count++;
	return newptr;
}

void *btGenericPoolAllocator::failback_alloc(size_t size_bytes)
{
	btGenericMemoryPool *pool = NULL;

	if (size_bytes <= get_pool_capacity())
	{
		pool = push_new_pool();
	}

	if (pool == NULL)  
	{
		return btAlignedAlloc(size_bytes, 16);
	}

	return pool->allocate(size_bytes);
}

bool btGenericPoolAllocator::failback_free(void *pointer)
{
	btAlignedFree(pointer);
	return true;
}



void *btGenericPoolAllocator::allocate(size_t size_bytes)
{
	void *ptr = NULL;

	size_t i = 0;
	while (i < m_pool_count && ptr == NULL)
	{
		ptr = m_pools[i]->allocate(size_bytes);
		++i;
	}

	if (ptr) return ptr;

	return failback_alloc(size_bytes);
}

bool btGenericPoolAllocator::freeMemory(void *pointer)
{
	bool result = false;

	size_t i = 0;
	while (i < m_pool_count && result == false)
	{
		result = m_pools[i]->freeMemory(pointer);
		++i;
	}

	if (result) return true;

	return failback_free(pointer);
}



#define BT_DEFAULT_POOL_SIZE 32768
#define BT_DEFAULT_POOL_ELEMENT_SIZE 8


class GIM_STANDARD_ALLOCATOR : public btGenericPoolAllocator
{
public:
	GIM_STANDARD_ALLOCATOR() : btGenericPoolAllocator(BT_DEFAULT_POOL_ELEMENT_SIZE, BT_DEFAULT_POOL_SIZE)
	{
	}
};


GIM_STANDARD_ALLOCATOR g_main_allocator;

void *btPoolAlloc(size_t size)
{
	return g_main_allocator.allocate(size);
}

void *btPoolRealloc(void *ptr, size_t oldsize, size_t newsize)
{
	void *newptr = btPoolAlloc(newsize);
	size_t copysize = oldsize < newsize ? oldsize : newsize;
	memcpy(newptr, ptr, copysize);
	btPoolFree(ptr);
	return newptr;
}

void btPoolFree(void *ptr)
{
	g_main_allocator.freeMemory(ptr);
}
