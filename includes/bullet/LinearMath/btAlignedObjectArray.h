

#ifndef BT_OBJECT_ARRAY__
#define BT_OBJECT_ARRAY__

#include "btScalar.h"  
#include "btAlignedAllocator.h"







#define BT_USE_PLACEMENT_NEW 1

#define BT_ALLOW_ARRAY_COPY_OPERATOR  

#ifdef BT_USE_MEMCPY
#include <memory.h>
#include <string.h>
#endif  

#ifdef BT_USE_PLACEMENT_NEW
#include <new>  
#endif          



template <typename T>

class btAlignedObjectArray
{
	btAlignedAllocator<T, 16> m_allocator;

	int m_size;
	int m_capacity;
	T* m_data;
	
	bool m_ownsMemory;

#ifdef BT_ALLOW_ARRAY_COPY_OPERATOR
public:
	SIMD_FORCE_INLINE btAlignedObjectArray<T>& operator=(const btAlignedObjectArray<T>& other)
	{
		copyFromArray(other);
		return *this;
	}
#else   
private:
	SIMD_FORCE_INLINE btAlignedObjectArray<T>& operator=(const btAlignedObjectArray<T>& other);
#endif  

protected:
	SIMD_FORCE_INLINE int allocSize(int size)
	{
		return (size ? size * 2 : 1);
	}
	SIMD_FORCE_INLINE void copy(int start, int end, T* dest) const
	{
		int i;
		for (i = start; i < end; ++i)
#ifdef BT_USE_PLACEMENT_NEW
			new (&dest[i]) T(m_data[i]);
#else
			dest[i] = m_data[i];
#endif  
	}

	SIMD_FORCE_INLINE void init()
	{
		
		m_ownsMemory = true;
		m_data = 0;
		m_size = 0;
		m_capacity = 0;
	}
	SIMD_FORCE_INLINE void destroy(int first, int last)
	{
		int i;
		for (i = first; i < last; i++)
		{
			m_data[i].~T();
		}
	}

	SIMD_FORCE_INLINE void* allocate(int size)
	{
		if (size)
			return m_allocator.allocate(size);
		return 0;
	}

	SIMD_FORCE_INLINE void deallocate()
	{
		if (m_data)
		{
			
			if (m_ownsMemory)
			{
				m_allocator.deallocate(m_data);
			}
			m_data = 0;
		}
	}

public:
	btAlignedObjectArray()
	{
		init();
	}

	~btAlignedObjectArray()
	{
		clear();
	}

	
	btAlignedObjectArray(const btAlignedObjectArray& otherArray)
	{
		init();

		int otherSize = otherArray.size();
		resize(otherSize);
		otherArray.copy(0, otherSize, m_data);
	}

	
	SIMD_FORCE_INLINE int size() const
	{
		return m_size;
	}

	SIMD_FORCE_INLINE const T& at(int n) const
	{
		btAssert(n >= 0);
		btAssert(n < size());
		return m_data[n];
	}

	SIMD_FORCE_INLINE T& at(int n)
	{
		btAssert(n >= 0);
		btAssert(n < size());
		return m_data[n];
	}

	SIMD_FORCE_INLINE const T& operator[](int n) const
	{
		btAssert(n >= 0);
		btAssert(n < size());
		return m_data[n];
	}

	SIMD_FORCE_INLINE T& operator[](int n)
	{
		btAssert(n >= 0);
		btAssert(n < size());
		return m_data[n];
	}

	
	SIMD_FORCE_INLINE void clear()
	{
		destroy(0, size());

		deallocate();

		init();
	}

	SIMD_FORCE_INLINE void pop_back()
	{
		btAssert(m_size > 0);
		m_size--;
		m_data[m_size].~T();
	}

	
	
	SIMD_FORCE_INLINE void resizeNoInitialize(int newsize)
	{
		if (newsize > size())
		{
			reserve(newsize);
		}
		m_size = newsize;
	}

	SIMD_FORCE_INLINE void resize(int newsize, const T& fillData = T())
	{
		const int curSize = size();

		if (newsize < curSize)
		{
			for (int i = newsize; i < curSize; i++)
			{
				m_data[i].~T();
			}
		}
		else
		{
			if (newsize > curSize)
			{
				reserve(newsize);
			}
#ifdef BT_USE_PLACEMENT_NEW
			for (int i = curSize; i < newsize; i++)
			{
				new (&m_data[i]) T(fillData);
			}
#endif  
		}

		m_size = newsize;
	}
	SIMD_FORCE_INLINE T& expandNonInitializing()
	{
		const int sz = size();
		if (sz == capacity())
		{
			reserve(allocSize(size()));
		}
		m_size++;

		return m_data[sz];
	}

	SIMD_FORCE_INLINE T& expand(const T& fillValue = T())
	{
		const int sz = size();
		if (sz == capacity())
		{
			reserve(allocSize(size()));
		}
		m_size++;
#ifdef BT_USE_PLACEMENT_NEW
		new (&m_data[sz]) T(fillValue);  
#endif

		return m_data[sz];
	}

	SIMD_FORCE_INLINE void push_back(const T& _Val)
	{
		const int sz = size();
		if (sz == capacity())
		{
			reserve(allocSize(size()));
		}

#ifdef BT_USE_PLACEMENT_NEW
		new (&m_data[m_size]) T(_Val);
#else
		m_data[size()] = _Val;
#endif  

		m_size++;
	}

	
	SIMD_FORCE_INLINE int capacity() const
	{
		return m_capacity;
	}

	SIMD_FORCE_INLINE void reserve(int _Count)
	{  
		if (capacity() < _Count)
		{  
			T* s = (T*)allocate(_Count);

			copy(0, size(), s);

			destroy(0, size());

			deallocate();

			
			m_ownsMemory = true;

			m_data = s;

			m_capacity = _Count;
		}
	}

	class less
	{
	public:
		bool operator()(const T& a, const T& b) const
		{
			return (a < b);
		}
	};

	template <typename L>
	void quickSortInternal(const L& CompareFunc, int lo, int hi)
	{
		
		
		int i = lo, j = hi;
		T x = m_data[(lo + hi) / 2];

		
		do
		{
			while (CompareFunc(m_data[i], x))
				i++;
			while (CompareFunc(x, m_data[j]))
				j--;
			if (i <= j)
			{
				swap(i, j);
				i++;
				j--;
			}
		} while (i <= j);

		
		if (lo < j)
			quickSortInternal(CompareFunc, lo, j);
		if (i < hi)
			quickSortInternal(CompareFunc, i, hi);
	}

	template <typename L>
	void quickSort(const L& CompareFunc)
	{
		
		if (size() > 1)
		{
			quickSortInternal(CompareFunc, 0, size() - 1);
		}
	}

	
	template <typename L>
	void downHeap(T* pArr, int k, int n, const L& CompareFunc)
	{
		
		

		T temp = pArr[k - 1];
		
		while (k <= n / 2)
		{
			int child = 2 * k;

			if ((child < n) && CompareFunc(pArr[child - 1], pArr[child]))
			{
				child++;
			}
			
			if (CompareFunc(temp, pArr[child - 1]))
			{
				
				pArr[k - 1] = pArr[child - 1];
				k = child;
			}
			else
			{
				break;
			}
		}
		pArr[k - 1] = temp;
	} 

	void swap(int index0, int index1)
	{
#ifdef BT_USE_MEMCPY
		char temp[sizeof(T)];
		memcpy(temp, &m_data[index0], sizeof(T));
		memcpy(&m_data[index0], &m_data[index1], sizeof(T));
		memcpy(&m_data[index1], temp, sizeof(T));
#else
		T temp = m_data[index0];
		m_data[index0] = m_data[index1];
		m_data[index1] = temp;
#endif  
	}

	template <typename L>
	void heapSort(const L& CompareFunc)
	{
		
		int k;
		int n = m_size;
		for (k = n / 2; k > 0; k--)
		{
			downHeap(m_data, k, n, CompareFunc);
		}

		
		while (n >= 1)
		{
			swap(0, n - 1); 

			n = n - 1;
			
			downHeap(m_data, 1, n, CompareFunc);
		}
	}

	
	int findBinarySearch(const T& key) const
	{
		int first = 0;
		int last = size() - 1;

		
		while (first <= last)
		{
			int mid = (first + last) / 2;  
			if (key > m_data[mid])
				first = mid + 1;  
			else if (key < m_data[mid])
				last = mid - 1;  
			else
				return mid;  
		}
		return size();  
	}

	int findLinearSearch(const T& key) const
	{
		int index = size();
		int i;

		for (i = 0; i < size(); i++)
		{
			if (m_data[i] == key)
			{
				index = i;
				break;
			}
		}
		return index;
	}

	
	
	int findLinearSearch2(const T& key) const
	{
		int index = -1;
		int i;

		for (i = 0; i < size(); i++)
		{
			if (m_data[i] == key)
			{
				index = i;
				break;
			}
		}
		return index;
	}

	void removeAtIndex(int index)
	{
		if (index < size())
		{
			swap(index, size() - 1);
			pop_back();
		}
	}
	void remove(const T& key)
	{
		int findIndex = findLinearSearch(key);
		removeAtIndex(findIndex);
	}

	
	void initializeFromBuffer(void* buffer, int size, int capacity)
	{
		clear();
		m_ownsMemory = false;
		m_data = (T*)buffer;
		m_size = size;
		m_capacity = capacity;
	}

	void copyFromArray(const btAlignedObjectArray& otherArray)
	{
		int otherSize = otherArray.size();
		resize(otherSize);
		otherArray.copy(0, otherSize, m_data);
	}
};

#endif  
