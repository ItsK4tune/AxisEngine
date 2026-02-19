

#include "btAlignedAllocator.h"
#include <string.h>

#ifdef BT_DEBUG_MEMORY_ALLOCATIONS
int gNumAlignedAllocs = 0;
int gNumAlignedFree = 0;
int gTotalBytesAlignedAllocs = 0;  
#endif                             

static void *btAllocDefault(size_t size)
{
  char* data = (char*) malloc(size);
  memset(data,0,size);
  return data;
}

static void btFreeDefault(void *ptr)
{
	free(ptr);
}

static btAllocFunc *sAllocFunc = btAllocDefault;
static btFreeFunc *sFreeFunc = btFreeDefault;

#if defined(BT_HAS_ALIGNED_ALLOCATOR)
#include <malloc.h>
static void *btAlignedAllocDefault(size_t size, int alignment)
{
	return _aligned_malloc(size, (size_t)alignment);
}

static void btAlignedFreeDefault(void *ptr)
{
	_aligned_free(ptr);
}
#elif defined(__CELLOS_LV2__)
#include <stdlib.h>

static inline void *btAlignedAllocDefault(size_t size, int alignment)
{
	return memalign(alignment, size);
}

static inline void btAlignedFreeDefault(void *ptr)
{
	free(ptr);
}
#else

static inline void *btAlignedAllocDefault(size_t size, int alignment)
{
	void *ret;
	char *real;
	real = (char *)sAllocFunc(size + sizeof(void *) + (alignment - 1));
	if (real)
	{
		ret = btAlignPointer(real + sizeof(void *), alignment);
		*((void **)(ret)-1) = (void *)(real);
	}
	else
	{
		ret = (void *)(real);
	}
  
  memset((char*) ret, 0, size);
	return (ret);
}

static inline void btAlignedFreeDefault(void *ptr)
{
	void *real;

	if (ptr)
	{
		real = *((void **)(ptr)-1);
		sFreeFunc(real);
	}
}
#endif

static btAlignedAllocFunc *sAlignedAllocFunc = btAlignedAllocDefault;
static btAlignedFreeFunc *sAlignedFreeFunc = btAlignedFreeDefault;

void btAlignedAllocSetCustomAligned(btAlignedAllocFunc *allocFunc, btAlignedFreeFunc *freeFunc)
{
	sAlignedAllocFunc = allocFunc ? allocFunc : btAlignedAllocDefault;
	sAlignedFreeFunc = freeFunc ? freeFunc : btAlignedFreeDefault;
}

void btAlignedAllocSetCustom(btAllocFunc *allocFunc, btFreeFunc *freeFunc)
{
	sAllocFunc = allocFunc ? allocFunc : btAllocDefault;
	sFreeFunc = freeFunc ? freeFunc : btFreeDefault;
}

#ifdef BT_DEBUG_MEMORY_ALLOCATIONS

static int allocations_id[10241024];
static int allocations_bytes[10241024];
static int mynumallocs = 0;
#include <stdio.h>

int btDumpMemoryLeaks()
{
	int totalLeak = 0;

	for (int i = 0; i < mynumallocs; i++)
	{
		printf("Error: leaked memory of allocation #%d (%d bytes)\n", allocations_id[i], allocations_bytes[i]);
		totalLeak += allocations_bytes[i];
	}
	if (totalLeak)
	{
		printf("Error: memory leaks: %d allocations were not freed and leaked together %d bytes\n", mynumallocs, totalLeak);
	}
	return totalLeak;
}

#include <stdio.h>

struct btDebugPtrMagic
{
	union {
		void **vptrptr;
		void *vptr;
		int *iptr;
		char *cptr;
	};
};

void *btAlignedAllocInternal(size_t size, int alignment, int line, const char *filename)
{
	if (size == 0)
	{
		printf("Whaat? size==0");
		return 0;
	}
	static int allocId = 0;

	void *ret;
	char *real;

	
	
	
	
	
	
	
	
	

	gTotalBytesAlignedAllocs += size;
	gNumAlignedAllocs++;

	int sz4prt = 4 * sizeof(void *);

	real = (char *)sAllocFunc(size + sz4prt + (alignment - 1));
	if (real)
	{
		ret = (void *)btAlignPointer(real + sz4prt, alignment);
		btDebugPtrMagic p;
		p.vptr = ret;
		p.cptr -= sizeof(void *);
		*p.vptrptr = (void *)real;
		p.cptr -= sizeof(void *);
		*p.iptr = size;
		p.cptr -= sizeof(void *);
		*p.iptr = allocId;

		allocations_id[mynumallocs] = allocId;
		allocations_bytes[mynumallocs] = size;
		mynumallocs++;
	}
	else
	{
		ret = (void *)(real);  
	}

	printf("allocation %d at address %x, from %s,line %d, size %d (total allocated = %d)\n", allocId, real, filename, line, size, gTotalBytesAlignedAllocs);
	allocId++;

	int *ptr = (int *)ret;
	*ptr = 12;
	return (ret);
}

void btAlignedFreeInternal(void *ptr, int line, const char *filename)
{
	void *real;

	if (ptr)
	{
		gNumAlignedFree++;

		btDebugPtrMagic p;
		p.vptr = ptr;
		p.cptr -= sizeof(void *);
		real = *p.vptrptr;
		p.cptr -= sizeof(void *);
		int size = *p.iptr;
		p.cptr -= sizeof(void *);
		int allocId = *p.iptr;

		bool found = false;

		for (int i = 0; i < mynumallocs; i++)
		{
			if (allocations_id[i] == allocId)
			{
				allocations_id[i] = allocations_id[mynumallocs - 1];
				allocations_bytes[i] = allocations_bytes[mynumallocs - 1];
				mynumallocs--;
				found = true;
				break;
			}
		}

		gTotalBytesAlignedAllocs -= size;

		int diff = gNumAlignedAllocs - gNumAlignedFree;
		printf("free %d at address %x, from %s,line %d, size %d (total remain = %d in %d non-freed allocations)\n", allocId, real, filename, line, size, gTotalBytesAlignedAllocs, diff);

		sFreeFunc(real);
	}
	else
	{
		
	}
}

#else  

void *btAlignedAllocInternal(size_t size, int alignment)
{
	void *ptr;
	ptr = sAlignedAllocFunc(size, alignment);
	
	return ptr;
}

void btAlignedFreeInternal(void *ptr)
{
	if (!ptr)
	{
		return;
	}

	
	sAlignedFreeFunc(ptr);
}

#endif  
