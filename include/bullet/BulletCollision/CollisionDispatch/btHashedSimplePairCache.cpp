

#include "btHashedSimplePairCache.h"

#include <stdio.h>

#ifdef BT_DEBUG_COLLISION_PAIRS
int gOverlappingSimplePairs = 0;
int gRemoveSimplePairs = 0;
int gAddedSimplePairs = 0;
int gFindSimplePairs = 0;
#endif  

btHashedSimplePairCache::btHashedSimplePairCache()
{
	int initialAllocatedSize = 2;
	m_overlappingPairArray.reserve(initialAllocatedSize);
	growTables();
}

btHashedSimplePairCache::~btHashedSimplePairCache()
{
}

void btHashedSimplePairCache::removeAllPairs()
{
	m_overlappingPairArray.clear();
	m_hashTable.clear();
	m_next.clear();

	int initialAllocatedSize = 2;
	m_overlappingPairArray.reserve(initialAllocatedSize);
	growTables();
}

btSimplePair* btHashedSimplePairCache::findPair(int indexA, int indexB)
{
#ifdef BT_DEBUG_COLLISION_PAIRS
	gFindSimplePairs++;
#endif

	

	int hash = static_cast<int>(getHash(static_cast<unsigned int>(indexA), static_cast<unsigned int>(indexB)) & (m_overlappingPairArray.capacity() - 1));

	if (hash >= m_hashTable.size())
	{
		return NULL;
	}

	int index = m_hashTable[hash];
	while (index != BT_SIMPLE_NULL_PAIR && equalsPair(m_overlappingPairArray[index], indexA, indexB) == false)
	{
		index = m_next[index];
	}

	if (index == BT_SIMPLE_NULL_PAIR)
	{
		return NULL;
	}

	btAssert(index < m_overlappingPairArray.size());

	return &m_overlappingPairArray[index];
}



void btHashedSimplePairCache::growTables()
{
	int newCapacity = m_overlappingPairArray.capacity();

	if (m_hashTable.size() < newCapacity)
	{
		
		int curHashtableSize = m_hashTable.size();

		m_hashTable.resize(newCapacity);
		m_next.resize(newCapacity);

		int i;

		for (i = 0; i < newCapacity; ++i)
		{
			m_hashTable[i] = BT_SIMPLE_NULL_PAIR;
		}
		for (i = 0; i < newCapacity; ++i)
		{
			m_next[i] = BT_SIMPLE_NULL_PAIR;
		}

		for (i = 0; i < curHashtableSize; i++)
		{
			const btSimplePair& pair = m_overlappingPairArray[i];
			int indexA = pair.m_indexA;
			int indexB = pair.m_indexB;

			int hashValue = static_cast<int>(getHash(static_cast<unsigned int>(indexA), static_cast<unsigned int>(indexB)) & (m_overlappingPairArray.capacity() - 1));  
			m_next[i] = m_hashTable[hashValue];
			m_hashTable[hashValue] = i;
		}
	}
}

btSimplePair* btHashedSimplePairCache::internalAddPair(int indexA, int indexB)
{
	int hash = static_cast<int>(getHash(static_cast<unsigned int>(indexA), static_cast<unsigned int>(indexB)) & (m_overlappingPairArray.capacity() - 1));  

	btSimplePair* pair = internalFindPair(indexA, indexB, hash);
	if (pair != NULL)
	{
		return pair;
	}

	int count = m_overlappingPairArray.size();
	int oldCapacity = m_overlappingPairArray.capacity();
	void* mem = &m_overlappingPairArray.expandNonInitializing();

	int newCapacity = m_overlappingPairArray.capacity();

	if (oldCapacity < newCapacity)
	{
		growTables();
		
		hash = static_cast<int>(getHash(static_cast<unsigned int>(indexA), static_cast<unsigned int>(indexB)) & (m_overlappingPairArray.capacity() - 1));
	}

	pair = new (mem) btSimplePair(indexA, indexB);

	pair->m_userPointer = 0;

	m_next[count] = m_hashTable[hash];
	m_hashTable[hash] = count;

	return pair;
}

void* btHashedSimplePairCache::removeOverlappingPair(int indexA, int indexB)
{
#ifdef BT_DEBUG_COLLISION_PAIRS
	gRemoveSimplePairs++;
#endif

	

	int hash = static_cast<int>(getHash(static_cast<unsigned int>(indexA), static_cast<unsigned int>(indexB)) & (m_overlappingPairArray.capacity() - 1));

	btSimplePair* pair = internalFindPair(indexA, indexB, hash);
	if (pair == NULL)
	{
		return 0;
	}

	void* userData = pair->m_userPointer;

	int pairIndex = int(pair - &m_overlappingPairArray[0]);
	btAssert(pairIndex < m_overlappingPairArray.size());

	
	int index = m_hashTable[hash];
	btAssert(index != BT_SIMPLE_NULL_PAIR);

	int previous = BT_SIMPLE_NULL_PAIR;
	while (index != pairIndex)
	{
		previous = index;
		index = m_next[index];
	}

	if (previous != BT_SIMPLE_NULL_PAIR)
	{
		btAssert(m_next[previous] == pairIndex);
		m_next[previous] = m_next[pairIndex];
	}
	else
	{
		m_hashTable[hash] = m_next[pairIndex];
	}

	
	
	

	int lastPairIndex = m_overlappingPairArray.size() - 1;

	
	if (lastPairIndex == pairIndex)
	{
		m_overlappingPairArray.pop_back();
		return userData;
	}

	
	const btSimplePair* last = &m_overlappingPairArray[lastPairIndex];
	
	int lastHash = static_cast<int>(getHash(static_cast<unsigned int>(last->m_indexA), static_cast<unsigned int>(last->m_indexB)) & (m_overlappingPairArray.capacity() - 1));

	index = m_hashTable[lastHash];
	btAssert(index != BT_SIMPLE_NULL_PAIR);

	previous = BT_SIMPLE_NULL_PAIR;
	while (index != lastPairIndex)
	{
		previous = index;
		index = m_next[index];
	}

	if (previous != BT_SIMPLE_NULL_PAIR)
	{
		btAssert(m_next[previous] == lastPairIndex);
		m_next[previous] = m_next[lastPairIndex];
	}
	else
	{
		m_hashTable[lastHash] = m_next[lastPairIndex];
	}

	
	m_overlappingPairArray[pairIndex] = m_overlappingPairArray[lastPairIndex];

	
	m_next[pairIndex] = m_hashTable[lastHash];
	m_hashTable[lastHash] = pairIndex;

	m_overlappingPairArray.pop_back();

	return userData;
}

