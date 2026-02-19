#ifndef GIM_ARRAY_H_INCLUDED
#define GIM_ARRAY_H_INCLUDED



#include "gim_memory.h"

#define GIM_ARRAY_GROW_INCREMENT 2
#define GIM_ARRAY_GROW_FACTOR 2


template <typename T>
class gim_array
{
public:
	
	
	T* m_data;
	GUINT m_size;
	GUINT m_allocated_size;
	
	
	

	inline void destroyData()
	{
		m_allocated_size = 0;
		if (m_data == NULL) return;
		gim_free(m_data);
		m_data = NULL;
	}

	inline bool resizeData(GUINT newsize)
	{
		if (newsize == 0)
		{
			destroyData();
			return true;
		}

		if (m_size > 0)
		{
			m_data = (T*)gim_realloc(m_data, m_size * sizeof(T), newsize * sizeof(T));
		}
		else
		{
			m_data = (T*)gim_alloc(newsize * sizeof(T));
		}
		m_allocated_size = newsize;
		return true;
	}

	inline bool growingCheck()
	{
		if (m_allocated_size <= m_size)
		{
			GUINT requestsize = m_size;
			m_size = m_allocated_size;
			if (resizeData((requestsize + GIM_ARRAY_GROW_INCREMENT) * GIM_ARRAY_GROW_FACTOR) == false) return false;
		}
		return true;
	}

	
	
	
	inline bool reserve(GUINT size)
	{
		if (m_allocated_size >= size) return false;
		return resizeData(size);
	}

	inline void clear_range(GUINT start_range)
	{
		while (m_size > start_range)
		{
			m_data[--m_size].~T();
		}
	}

	inline void clear()
	{
		if (m_size == 0) return;
		clear_range(0);
	}

	inline void clear_memory()
	{
		clear();
		destroyData();
	}

	gim_array()
	{
		m_data = 0;
		m_size = 0;
		m_allocated_size = 0;
	}

	gim_array(GUINT reservesize)
	{
		m_data = 0;
		m_size = 0;

		m_allocated_size = 0;
		reserve(reservesize);
	}

	~gim_array()
	{
		clear_memory();
	}

	inline GUINT size() const
	{
		return m_size;
	}

	inline GUINT max_size() const
	{
		return m_allocated_size;
	}

	inline T& operator[](size_t i)
	{
		return m_data[i];
	}
	inline const T& operator[](size_t i) const
	{
		return m_data[i];
	}

	inline T* pointer() { return m_data; }
	inline const T* pointer() const
	{
		return m_data;
	}

	inline T* get_pointer_at(GUINT i)
	{
		return m_data + i;
	}

	inline const T* get_pointer_at(GUINT i) const
	{
		return m_data + i;
	}

	inline T& at(GUINT i)
	{
		return m_data[i];
	}

	inline const T& at(GUINT i) const
	{
		return m_data[i];
	}

	inline T& front()
	{
		return *m_data;
	}

	inline const T& front() const
	{
		return *m_data;
	}

	inline T& back()
	{
		return m_data[m_size - 1];
	}

	inline const T& back() const
	{
		return m_data[m_size - 1];
	}

	inline void swap(GUINT i, GUINT j)
	{
		gim_swap_elements(m_data, i, j);
	}

	inline void push_back(const T& obj)
	{
		this->growingCheck();
		m_data[m_size] = obj;
		m_size++;
	}

	
	inline void push_back_mem()
	{
		this->growingCheck();
		m_size++;
	}

	inline void push_back_memcpy(const T& obj)
	{
		this->growingCheck();
		gim_simd_memcpy(&m_data[m_size], &obj, sizeof(T));
		m_size++;
	}

	inline void pop_back()
	{
		m_size--;
		m_data[m_size].~T();
	}

	
	inline void pop_back_mem()
	{
		m_size--;
	}

	
	inline void erase(GUINT index)
	{
		if (index < m_size - 1)
		{
			swap(index, m_size - 1);
		}
		pop_back();
	}

	inline void erase_sorted_mem(GUINT index)
	{
		m_size--;
		for (GUINT i = index; i < m_size; i++)
		{
			gim_simd_memcpy(m_data + i, m_data + i + 1, sizeof(T));
		}
	}

	inline void erase_sorted(GUINT index)
	{
		m_data[index].~T();
		erase_sorted_mem(index);
	}

	inline void insert_mem(GUINT index)
	{
		this->growingCheck();
		for (GUINT i = m_size; i > index; i--)
		{
			gim_simd_memcpy(m_data + i, m_data + i - 1, sizeof(T));
		}
		m_size++;
	}

	inline void insert(const T& obj, GUINT index)
	{
		insert_mem(index);
		m_data[index] = obj;
	}

	inline void resize(GUINT size, bool call_constructor = true, const T& fillData = T())
	{
		if (size > m_size)
		{
			reserve(size);
			if (call_constructor)
			{
				while (m_size < size)
				{
					m_data[m_size] = fillData;
					m_size++;
				}
			}
			else
			{
				m_size = size;
			}
		}
		else if (size < m_size)
		{
			if (call_constructor) clear_range(size);
			m_size = size;
		}
	}

	inline void refit()
	{
		resizeData(m_size);
	}
};

#endif  
