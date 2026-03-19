

#ifndef BT_UNION_FIND_H
#define BT_UNION_FIND_H

#include "LinearMath/btAlignedObjectArray.h"

#define USE_PATH_COMPRESSION 1


#define STATIC_SIMULATION_ISLAND_OPTIMIZATION 1

struct btElement
{
	int m_id;
	int m_sz;
};




class btUnionFind
{
private:
	btAlignedObjectArray<btElement> m_elements;

public:
	btUnionFind();
	~btUnionFind();

	
	
	void sortIslands();

	void reset(int N);

	SIMD_FORCE_INLINE int getNumElements() const
	{
		return int(m_elements.size());
	}
	SIMD_FORCE_INLINE bool isRoot(int x) const
	{
		return (x == m_elements[x].m_id);
	}

	btElement& getElement(int index)
	{
		return m_elements[index];
	}
	const btElement& getElement(int index) const
	{
		return m_elements[index];
	}

	void allocate(int N);
	void Free();

	int find(int p, int q)
	{
		return (find(p) == find(q));
	}

	void unite(int p, int q)
	{
		int i = find(p), j = find(q);
		if (i == j)
			return;

#ifndef USE_PATH_COMPRESSION
		
		if (m_elements[i].m_sz < m_elements[j].m_sz)
		{
			m_elements[i].m_id = j;
			m_elements[j].m_sz += m_elements[i].m_sz;
		}
		else
		{
			m_elements[j].m_id = i;
			m_elements[i].m_sz += m_elements[j].m_sz;
		}
#else
		m_elements[i].m_id = j;
		m_elements[j].m_sz += m_elements[i].m_sz;
#endif  
	}

	int find(int x)
	{
		
		

		while (x != m_elements[x].m_id)
		{
			

#ifdef USE_PATH_COMPRESSION
			const btElement* elementPtr = &m_elements[m_elements[x].m_id];
			m_elements[x].m_id = elementPtr->m_id;
			x = elementPtr->m_id;
#else  
			x = m_elements[x].m_id;
#endif
			
			
		}
		return x;
	}
};

#endif  
