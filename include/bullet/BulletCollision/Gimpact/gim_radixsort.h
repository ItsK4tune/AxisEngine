#ifndef GIM_RADIXSORT_H_INCLUDED
#define GIM_RADIXSORT_H_INCLUDED



#include "gim_memory.h"



class less_comparator
{
public:
	template <class T, class Z>
	inline int operator()(const T& a, const Z& b)
	{
		return (a < b ? -1 : (a > b ? 1 : 0));
	}
};


class integer_comparator
{
public:
	template <class T>
	inline int operator()(const T& a, const T& b)
	{
		return (int)(a - b);
	}
};


class uint_key_func
{
public:
	template <class T>
	inline GUINT operator()(const T& a)
	{
		return (GUINT)a;
	}
};


class copy_elements_func
{
public:
	template <class T>
	inline void operator()(T& a, T& b)
	{
		a = b;
	}
};


class memcopy_elements_func
{
public:
	template <class T>
	inline void operator()(T& a, T& b)
	{
		gim_simd_memcpy(&a, &b, sizeof(T));
	}
};


struct GIM_RSORT_TOKEN
{
	GUINT m_key;
	GUINT m_value;
	GIM_RSORT_TOKEN()
	{
	}
	GIM_RSORT_TOKEN(const GIM_RSORT_TOKEN& rtoken)
	{
		m_key = rtoken.m_key;
		m_value = rtoken.m_value;
	}

	inline bool operator<(const GIM_RSORT_TOKEN& other) const
	{
		return (m_key < other.m_key);
	}

	inline bool operator>(const GIM_RSORT_TOKEN& other) const
	{
		return (m_key > other.m_key);
	}
};


class GIM_RSORT_TOKEN_COMPARATOR
{
public:
	inline int operator()(const GIM_RSORT_TOKEN& a, const GIM_RSORT_TOKEN& b)
	{
		return (int)((a.m_key) - (b.m_key));
	}
};

#define kHist 2048

#define D11_0(x) (x & 0x7FF)
#define D11_1(x) (x >> 11 & 0x7FF)
#define D11_2(x) (x >> 22)


inline void gim_radix_sort_rtokens(
	GIM_RSORT_TOKEN* array,
	GIM_RSORT_TOKEN* sorted, GUINT element_count)
{
	GUINT i;
	GUINT b0[kHist * 3];
	GUINT* b1 = b0 + kHist;
	GUINT* b2 = b1 + kHist;
	for (i = 0; i < kHist * 3; ++i)
	{
		b0[i] = 0;
	}
	GUINT fi;
	GUINT pos;
	for (i = 0; i < element_count; ++i)
	{
		fi = array[i].m_key;
		b0[D11_0(fi)]++;
		b1[D11_1(fi)]++;
		b2[D11_2(fi)]++;
	}
	{
		GUINT sum0 = 0, sum1 = 0, sum2 = 0;
		GUINT tsum;
		for (i = 0; i < kHist; ++i)
		{
			tsum = b0[i] + sum0;
			b0[i] = sum0 - 1;
			sum0 = tsum;
			tsum = b1[i] + sum1;
			b1[i] = sum1 - 1;
			sum1 = tsum;
			tsum = b2[i] + sum2;
			b2[i] = sum2 - 1;
			sum2 = tsum;
		}
	}
	for (i = 0; i < element_count; ++i)
	{
		fi = array[i].m_key;
		pos = D11_0(fi);
		pos = ++b0[pos];
		sorted[pos].m_key = array[i].m_key;
		sorted[pos].m_value = array[i].m_value;
	}
	for (i = 0; i < element_count; ++i)
	{
		fi = sorted[i].m_key;
		pos = D11_1(fi);
		pos = ++b1[pos];
		array[pos].m_key = sorted[i].m_key;
		array[pos].m_value = sorted[i].m_value;
	}
	for (i = 0; i < element_count; ++i)
	{
		fi = array[i].m_key;
		pos = D11_2(fi);
		pos = ++b2[pos];
		sorted[pos].m_key = array[i].m_key;
		sorted[pos].m_value = array[i].m_value;
	}
}



template <typename T, class GETKEY_CLASS>
void gim_radix_sort_array_tokens(
	T* array,
	GIM_RSORT_TOKEN* sorted_tokens,
	GUINT element_count, GETKEY_CLASS uintkey_macro)
{
	GIM_RSORT_TOKEN* _unsorted = (GIM_RSORT_TOKEN*)gim_alloc(sizeof(GIM_RSORT_TOKEN) * element_count);
	for (GUINT _i = 0; _i < element_count; ++_i)
	{
		_unsorted[_i].m_key = uintkey_macro(array[_i]);
		_unsorted[_i].m_value = _i;
	}
	gim_radix_sort_rtokens(_unsorted, sorted_tokens, element_count);
	gim_free(_unsorted);
	gim_free(_unsorted);
}



template <typename T, class GETKEY_CLASS, class COPY_CLASS>
void gim_radix_sort(
	T* array, GUINT element_count,
	GETKEY_CLASS get_uintkey_macro, COPY_CLASS copy_elements_macro)
{
	GIM_RSORT_TOKEN* _sorted = (GIM_RSORT_TOKEN*)gim_alloc(sizeof(GIM_RSORT_TOKEN) * element_count);
	gim_radix_sort_array_tokens(array, _sorted, element_count, get_uintkey_macro);
	T* _original_array = (T*)gim_alloc(sizeof(T) * element_count);
	gim_simd_memcpy(_original_array, array, sizeof(T) * element_count);
	for (GUINT _i = 0; _i < element_count; ++_i)
	{
		copy_elements_macro(array[_i], _original_array[_sorted[_i].m_value]);
	}
	gim_free(_original_array);
	gim_free(_sorted);
}



template <class T, typename KEYCLASS, typename COMP_CLASS>
bool gim_binary_search_ex(
	const T* _array, GUINT _start_i,
	GUINT _end_i, GUINT& _result_index,
	const KEYCLASS& _search_key,
	COMP_CLASS _comp_macro)
{
	GUINT _k;
	int _comp_result;
	GUINT _i = _start_i;
	GUINT _j = _end_i + 1;
	while (_i < _j)
	{
		_k = (_j + _i - 1) / 2;
		_comp_result = _comp_macro(_array[_k], _search_key);
		if (_comp_result == 0)
		{
			_result_index = _k;
			return true;
		}
		else if (_comp_result < 0)
		{
			_i = _k + 1;
		}
		else
		{
			_j = _k;
		}
	}
	_result_index = _i;
	return false;
}



template <class T>
bool gim_binary_search(
	const T* _array, GUINT _start_i,
	GUINT _end_i, const T& _search_key,
	GUINT& _result_index)
{
	GUINT _i = _start_i;
	GUINT _j = _end_i + 1;
	GUINT _k;
	while (_i < _j)
	{
		_k = (_j + _i - 1) / 2;
		if (_array[_k] == _search_key)
		{
			_result_index = _k;
			return true;
		}
		else if (_array[_k] < _search_key)
		{
			_i = _k + 1;
		}
		else
		{
			_j = _k;
		}
	}
	_result_index = _i;
	return false;
}


template <typename T, typename COMP_CLASS>
void gim_down_heap(T* pArr, GUINT k, GUINT n, COMP_CLASS CompareFunc)
{
	
	

	T temp = pArr[k - 1];
	
	while (k <= n / 2)
	{
		int child = 2 * k;

		if ((child < (int)n) && CompareFunc(pArr[child - 1], pArr[child]) < 0)
		{
			child++;
		}
		
		if (CompareFunc(temp, pArr[child - 1]) < 0)
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

template <typename T, typename COMP_CLASS>
void gim_heap_sort(T* pArr, GUINT element_count, COMP_CLASS CompareFunc)
{
	
	GUINT k;
	GUINT n = element_count;
	for (k = n / 2; k > 0; k--)
	{
		gim_down_heap(pArr, k, n, CompareFunc);
	}

	
	while (n >= 2)
	{
		gim_swap_elements(pArr, 0, n - 1); 
		--n;
		
		gim_down_heap(pArr, 1, n, CompareFunc);
	}
}

#endif  
