

#ifndef BT_HASHED_SIMPLE_PAIR_CACHE_H
#define BT_HASHED_SIMPLE_PAIR_CACHE_H

#include "LinearMath/btAlignedObjectArray.h"

const int BT_SIMPLE_NULL_PAIR = 0xffffffff;

struct btSimplePair
{
	btSimplePair(int indexA, int indexB)
		: m_indexA(indexA),
		  m_indexB(indexB),
		  m_userPointer(0)
	{
	}

	int m_indexA;
	int m_indexB;
	union {
		void* m_userPointer;
		int m_userValue;
	};
};

typedef btAlignedObjectArray<btSimplePair> btSimplePairArray;

#ifdef BT_DEBUG_COLLISION_PAIRS
extern int gOverlappingSimplePairs;
extern int gRemoveSimplePairs;
extern int gAddedSimplePairs;
extern int gFindSimplePairs;
#endif  

class btHashedSimplePairCache
{
	btSimplePairArray m_overlappingPairArray;

protected:
	btAlignedObjectArray<int> m_hashTable;
	btAlignedObjectArray<int> m_next;

public:
	btHashedSimplePairCache();
	virtual ~btHashedSimplePairCache();

	void removeAllPairs();

	virtual void* removeOverlappingPair(int indexA, int indexB);

	
	
	virtual btSimplePair* addOverlappingPair(int indexA, int indexB)
	{
#ifdef BT_DEBUG_COLLISION_PAIRS
		gAddedSimplePairs++;
#endif

		return internalAddPair(indexA, indexB);
	}

	virtual btSimplePair* getOverlappingPairArrayPtr()
	{
		return &m_overlappingPairArray[0];
	}

	const btSimplePair* getOverlappingPairArrayPtr() const
	{
		return &m_overlappingPairArray[0];
	}

	btSimplePairArray& getOverlappingPairArray()
	{
		return m_overlappingPairArray;
	}

	const btSimplePairArray& getOverlappingPairArray() const
	{
		return m_overlappingPairArray;
	}

	btSimplePair* findPair(int indexA, int indexB);

	int GetCount() const { return m_overlappingPairArray.size(); }

	int getNumOverlappingPairs() const
	{
		return m_overlappingPairArray.size();
	}

private:
	btSimplePair* internalAddPair(int indexA, int indexB);

	void growTables();

	SIMD_FORCE_INLINE bool equalsPair(const btSimplePair& pair, int indexA, int indexB)
	{
		return pair.m_indexA == indexA && pair.m_indexB == indexB;
	}

	SIMD_FORCE_INLINE unsigned int getHash(unsigned int indexA, unsigned int indexB)
	{
		unsigned int key = indexA | (indexB << 16);
		

		key += ~(key << 15);
		key ^= (key >> 10);
		key += (key << 3);
		key ^= (key >> 6);
		key += ~(key << 11);
		key ^= (key >> 16);
		return key;
	}

	SIMD_FORCE_INLINE btSimplePair* internalFindPair(int proxyIdA, int proxyIdB, int hash)
	{
		int index = m_hashTable[hash];

		while (index != BT_SIMPLE_NULL_PAIR && equalsPair(m_overlappingPairArray[index], proxyIdA, proxyIdB) == false)
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
};

#endif  
