

#ifndef BT_ALIGNED_ALLOCATOR
#define BT_ALIGNED_ALLOCATOR





#include "btScalar.h"




#ifdef BT_DEBUG_MEMORY_ALLOCATIONS

int btDumpMemoryLeaks();

#define btAlignedAlloc(a, b) \
	btAlignedAllocInternal(a, b, __LINE__, __FILE__)

#define btAlignedFree(ptr) \
	btAlignedFreeInternal(ptr, __LINE__, __FILE__)

void* btAlignedAllocInternal(size_t size, int alignment, int line, const char* filename);

void btAlignedFreeInternal(void* ptr, int line, const char* filename);

#else
void* btAlignedAllocInternal(size_t size, int alignment);
void btAlignedFreeInternal(void* ptr);

#define btAlignedAlloc(size, alignment) btAlignedAllocInternal(size, alignment)
#define btAlignedFree(ptr) btAlignedFreeInternal(ptr)

#endif
typedef int size_type;

typedef void*(btAlignedAllocFunc)(size_t size, int alignment);
typedef void(btAlignedFreeFunc)(void* memblock);
typedef void*(btAllocFunc)(size_t size);
typedef void(btFreeFunc)(void* memblock);


void btAlignedAllocSetCustom(btAllocFunc* allocFunc, btFreeFunc* freeFunc);

void btAlignedAllocSetCustomAligned(btAlignedAllocFunc* allocFunc, btAlignedFreeFunc* freeFunc);



template <typename T, unsigned Alignment>
class btAlignedAllocator
{
	typedef btAlignedAllocator<T, Alignment> self_type;

public:
	
	btAlignedAllocator() {}
	

	template <typename Other>
	btAlignedAllocator(const btAlignedAllocator<Other, Alignment>&)
	{
	}

	typedef const T* const_pointer;
	typedef const T& const_reference;
	typedef T* pointer;
	typedef T& reference;
	typedef T value_type;

	pointer address(reference ref) const { return &ref; }
	const_pointer address(const_reference ref) const { return &ref; }
	pointer allocate(size_type n, const_pointer* hint = 0)
	{
		(void)hint;
		return reinterpret_cast<pointer>(btAlignedAlloc(sizeof(value_type) * n, Alignment));
	}
	void construct(pointer ptr, const value_type& value) { new (ptr) value_type(value); }
	void deallocate(pointer ptr)
	{
		btAlignedFree(reinterpret_cast<void*>(ptr));
	}
	void destroy(pointer ptr) { ptr->~value_type(); }

	template <typename O>
	struct rebind
	{
		typedef btAlignedAllocator<O, Alignment> other;
	};
	template <typename O>
	self_type& operator=(const btAlignedAllocator<O, Alignment>&)
	{
		return *this;
	}

	friend bool operator==(const self_type&, const self_type&) { return true; }
};

#endif  
