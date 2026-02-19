#ifndef GIM_BITSET_H_INCLUDED
#define GIM_BITSET_H_INCLUDED



#include "gim_array.h"

#define GUINT_BIT_COUNT 32
#define GUINT_EXPONENT 5

class gim_bitset
{
public:
	gim_array<GUINT> m_container;

	gim_bitset()
	{
	}

	gim_bitset(GUINT bits_count)
	{
		resize(bits_count);
	}

	~gim_bitset()
	{
	}

	inline bool resize(GUINT newsize)
	{
		GUINT oldsize = m_container.size();
		m_container.resize(newsize / GUINT_BIT_COUNT + 1, false);
		while (oldsize < m_container.size())
		{
			m_container[oldsize] = 0;
		}
		return true;
	}

	inline GUINT size()
	{
		return m_container.size() * GUINT_BIT_COUNT;
	}

	inline void set_all()
	{
		for (GUINT i = 0; i < m_container.size(); ++i)
		{
			m_container[i] = 0xffffffff;
		}
	}

	inline void clear_all()
	{
		for (GUINT i = 0; i < m_container.size(); ++i)
		{
			m_container[i] = 0;
		}
	}

	inline void set(GUINT bit_index)
	{
		if (bit_index >= size())
		{
			resize(bit_index);
		}
		m_container[bit_index >> GUINT_EXPONENT] |= (1 << (bit_index & (GUINT_BIT_COUNT - 1)));
	}

	
	inline char get(GUINT bit_index)
	{
		if (bit_index >= size())
		{
			return 0;
		}
		char value = m_container[bit_index >> GUINT_EXPONENT] &
					 (1 << (bit_index & (GUINT_BIT_COUNT - 1)));
		return value;
	}

	inline void clear(GUINT bit_index)
	{
		m_container[bit_index >> GUINT_EXPONENT] &= ~(1 << (bit_index & (GUINT_BIT_COUNT - 1)));
	}
};

#endif  
