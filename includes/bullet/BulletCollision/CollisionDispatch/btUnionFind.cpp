

#include "btUnionFind.h"

btUnionFind::~btUnionFind()
{
	Free();
}

btUnionFind::btUnionFind()
{
}

void btUnionFind::allocate(int N)
{
	m_elements.resize(N);
}
void btUnionFind::Free()
{
	m_elements.clear();
}

void btUnionFind::reset(int N)
{
	allocate(N);

	for (int i = 0; i < N; i++)
	{
		m_elements[i].m_id = i;
		m_elements[i].m_sz = 1;
	}
}

class btUnionFindElementSortPredicate
{
public:
	bool operator()(const btElement& lhs, const btElement& rhs) const
	{
		return lhs.m_id < rhs.m_id;
	}
};



void btUnionFind::sortIslands()
{
	
	int numElements = m_elements.size();

	for (int i = 0; i < numElements; i++)
	{
		m_elements[i].m_id = find(i);
#ifndef STATIC_SIMULATION_ISLAND_OPTIMIZATION
		m_elements[i].m_sz = i;
#endif  
	}

	
	
	m_elements.quickSort(btUnionFindElementSortPredicate());
}
