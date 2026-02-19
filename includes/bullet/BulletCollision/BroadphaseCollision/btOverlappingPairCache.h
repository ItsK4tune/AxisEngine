

#ifndef BT_OVERLAPPING_PAIR_CACHE_H
#define BT_OVERLAPPING_PAIR_CACHE_H

#include "btBroadphaseInterface.h"
#include "btBroadphaseProxy.h"
#include "btOverlappingPairCallback.h"

#include "LinearMath/btAlignedObjectArray.h"
class btDispatcher;

typedef btAlignedObjectArray<btBroadphasePair> btBroadphasePairArray;

struct btOverlapCallback
{
	virtual ~btOverlapCallback()
	{
	}
	
	virtual bool processOverlap(btBroadphasePair& pair) = 0;
};

struct btOverlapFilterCallback
{
	virtual ~btOverlapFilterCallback()
	{
	}
	
	virtual bool needBroadphaseCollision(btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1) const = 0;
};

const int BT_NULL_PAIR = 0xffffffff;



class btOverlappingPairCache : public btOverlappingPairCallback
{
public:
	virtual ~btOverlappingPairCache() {}  

	virtual btBroadphasePair* getOverlappingPairArrayPtr() = 0;

	virtual const btBroadphasePair* getOverlappingPairArrayPtr() const = 0;

	virtual btBroadphasePairArray& getOverlappingPairArray() = 0;

	virtual void cleanOverlappingPair(btBroadphasePair& pair, btDispatcher* dispatcher) = 0;

	virtual int getNumOverlappingPairs() const = 0;
	virtual bool needsBroadphaseCollision(btBroadphaseProxy * proxy0, btBroadphaseProxy * proxy1) const = 0;
	virtual btOverlapFilterCallback* getOverlapFilterCallback() = 0;
	virtual void cleanProxyFromPairs(btBroadphaseProxy* proxy, btDispatcher* dispatcher) = 0;

	virtual void setOverlapFilterCallback(btOverlapFilterCallback* callback) = 0;

	virtual void processAllOverlappingPairs(btOverlapCallback*, btDispatcher* dispatcher) = 0;

	virtual void processAllOverlappingPairs(btOverlapCallback* callback, btDispatcher* dispatcher, const struct btDispatcherInfo& )
	{
		processAllOverlappingPairs(callback, dispatcher);
	}
	virtual btBroadphasePair* findPair(btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1) = 0;

	virtual bool hasDeferredRemoval() = 0;

	virtual void setInternalGhostPairCallback(btOverlappingPairCallback* ghostPairCallback) = 0;

	virtual void sortOverlappingPairs(btDispatcher* dispatcher) = 0;
};



ATTRIBUTE_ALIGNED16(class)
btHashedOverlappingPairCache : public btOverlappingPairCache
{
	btBroadphasePairArray m_overlappingPairArray;
	btOverlapFilterCallback* m_overlapFilterCallback;

protected:
	btAlignedObjectArray<int> m_hashTable;
	btAlignedObjectArray<int> m_next;
	btOverlappingPairCallback* m_ghostPairCallback;

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btHashedOverlappingPairCache();
	virtual ~btHashedOverlappingPairCache();

	void removeOverlappingPairsContainingProxy(btBroadphaseProxy * proxy, btDispatcher * dispatcher);

	virtual void* removeOverlappingPair(btBroadphaseProxy * proxy0, btBroadphaseProxy * proxy1, btDispatcher * dispatcher);

	SIMD_FORCE_INLINE bool needsBroadphaseCollision(btBroadphaseProxy * proxy0, btBroadphaseProxy * proxy1) const
	{
		if (m_overlapFilterCallback)
			return m_overlapFilterCallback->needBroadphaseCollision(proxy0, proxy1);

		bool collides = (proxy0->m_collisionFilterGroup & proxy1->m_collisionFilterMask) != 0;
		collides = collides && (proxy1->m_collisionFilterGroup & proxy0->m_collisionFilterMask);

		return collides;
	}

	
	
	virtual btBroadphasePair* addOverlappingPair(btBroadphaseProxy * proxy0, btBroadphaseProxy * proxy1)
	{
		if (!needsBroadphaseCollision(proxy0, proxy1))
			return 0;

		return internalAddPair(proxy0, proxy1);
	}

	void cleanProxyFromPairs(btBroadphaseProxy * proxy, btDispatcher * dispatcher);

	virtual void processAllOverlappingPairs(btOverlapCallback*, btDispatcher * dispatcher);

	virtual void processAllOverlappingPairs(btOverlapCallback * callback, btDispatcher * dispatcher, const struct btDispatcherInfo& dispatchInfo);

	virtual btBroadphasePair* getOverlappingPairArrayPtr()
	{
		return &m_overlappingPairArray[0];
	}

	const btBroadphasePair* getOverlappingPairArrayPtr() const
	{
		return &m_overlappingPairArray[0];
	}

	btBroadphasePairArray& getOverlappingPairArray()
	{
		return m_overlappingPairArray;
	}

	const btBroadphasePairArray& getOverlappingPairArray() const
	{
		return m_overlappingPairArray;
	}

	void cleanOverlappingPair(btBroadphasePair & pair, btDispatcher * dispatcher);

	btBroadphasePair* findPair(btBroadphaseProxy * proxy0, btBroadphaseProxy * proxy1);

	int GetCount() const { return m_overlappingPairArray.size(); }
	

	btOverlapFilterCallback* getOverlapFilterCallback()
	{
		return m_overlapFilterCallback;
	}

	void setOverlapFilterCallback(btOverlapFilterCallback * callback)
	{
		m_overlapFilterCallback = callback;
	}

	int getNumOverlappingPairs() const
	{
		return m_overlappingPairArray.size();
	}

private:
	btBroadphasePair* internalAddPair(btBroadphaseProxy * proxy0, btBroadphaseProxy * proxy1);

	void growTables();

	SIMD_FORCE_INLINE bool equalsPair(const btBroadphasePair& pair, int proxyId1, int proxyId2)
	{
		return pair.m_pProxy0->getUid() == proxyId1 && pair.m_pProxy1->getUid() == proxyId2;
	}

	

	SIMD_FORCE_INLINE unsigned int getHash(unsigned int proxyId1, unsigned int proxyId2)
	{
		unsigned int key = proxyId1 | (proxyId2 << 16);
		

		key += ~(key << 15);
		key ^= (key >> 10);
		key += (key << 3);
		key ^= (key >> 6);
		key += ~(key << 11);
		key ^= (key >> 16);
		return key;
	}

	SIMD_FORCE_INLINE btBroadphasePair* internalFindPair(btBroadphaseProxy * proxy0, btBroadphaseProxy * proxy1, int hash)
	{
		int proxyId1 = proxy0->getUid();
		int proxyId2 = proxy1->getUid();
#if 0  
		if (proxyId1 > proxyId2) 
			btSwap(proxyId1, proxyId2);
#endif

		int index = m_hashTable[hash];

		while (index != BT_NULL_PAIR && equalsPair(m_overlappingPairArray[index], proxyId1, proxyId2) == false)
		{
			index = m_next[index];
		}

		if (index == BT_NULL_PAIR)
		{
			return NULL;
		}

		btAssert(index < m_overlappingPairArray.size());

		return &m_overlappingPairArray[index];
	}

	virtual bool hasDeferredRemoval()
	{
		return false;
	}

	virtual void setInternalGhostPairCallback(btOverlappingPairCallback * ghostPairCallback)
	{
		m_ghostPairCallback = ghostPairCallback;
	}

	virtual void sortOverlappingPairs(btDispatcher * dispatcher);
};



class btSortedOverlappingPairCache : public btOverlappingPairCache
{
protected:
	
	btBroadphasePairArray m_overlappingPairArray;

	
	bool m_blockedForChanges;

	
	bool m_hasDeferredRemoval;

	
	btOverlapFilterCallback* m_overlapFilterCallback;

	btOverlappingPairCallback* m_ghostPairCallback;

public:
	btSortedOverlappingPairCache();
	virtual ~btSortedOverlappingPairCache();

	virtual void processAllOverlappingPairs(btOverlapCallback*, btDispatcher* dispatcher);

	void* removeOverlappingPair(btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1, btDispatcher* dispatcher);

	void cleanOverlappingPair(btBroadphasePair& pair, btDispatcher* dispatcher);

	btBroadphasePair* addOverlappingPair(btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1);

	btBroadphasePair* findPair(btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1);

	void cleanProxyFromPairs(btBroadphaseProxy* proxy, btDispatcher* dispatcher);

	void removeOverlappingPairsContainingProxy(btBroadphaseProxy* proxy, btDispatcher* dispatcher);

	inline bool needsBroadphaseCollision(btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1) const
	{
		if (m_overlapFilterCallback)
			return m_overlapFilterCallback->needBroadphaseCollision(proxy0, proxy1);

		bool collides = (proxy0->m_collisionFilterGroup & proxy1->m_collisionFilterMask) != 0;
		collides = collides && (proxy1->m_collisionFilterGroup & proxy0->m_collisionFilterMask);

		return collides;
	}

	btBroadphasePairArray& getOverlappingPairArray()
	{
		return m_overlappingPairArray;
	}

	const btBroadphasePairArray& getOverlappingPairArray() const
	{
		return m_overlappingPairArray;
	}

	btBroadphasePair* getOverlappingPairArrayPtr()
	{
		return &m_overlappingPairArray[0];
	}

	const btBroadphasePair* getOverlappingPairArrayPtr() const
	{
		return &m_overlappingPairArray[0];
	}

	int getNumOverlappingPairs() const
	{
		return m_overlappingPairArray.size();
	}

	btOverlapFilterCallback* getOverlapFilterCallback()
	{
		return m_overlapFilterCallback;
	}

	void setOverlapFilterCallback(btOverlapFilterCallback* callback)
	{
		m_overlapFilterCallback = callback;
	}

	virtual bool hasDeferredRemoval()
	{
		return m_hasDeferredRemoval;
	}

	virtual void setInternalGhostPairCallback(btOverlappingPairCallback* ghostPairCallback)
	{
		m_ghostPairCallback = ghostPairCallback;
	}

	virtual void sortOverlappingPairs(btDispatcher* dispatcher);
};


class btNullPairCache : public btOverlappingPairCache
{
	btBroadphasePairArray m_overlappingPairArray;

public:
	virtual btBroadphasePair* getOverlappingPairArrayPtr()
	{
		return &m_overlappingPairArray[0];
	}
	const btBroadphasePair* getOverlappingPairArrayPtr() const
	{
		return &m_overlappingPairArray[0];
	}
	btBroadphasePairArray& getOverlappingPairArray()
	{
		return m_overlappingPairArray;
	}

	virtual void cleanOverlappingPair(btBroadphasePair& , btDispatcher* )
	{
	}

	virtual int getNumOverlappingPairs() const
	{
		return 0;
	}

	virtual void cleanProxyFromPairs(btBroadphaseProxy* , btDispatcher* )
	{
	}

	bool needsBroadphaseCollision(btBroadphaseProxy*, btBroadphaseProxy*) const
	{
		return true;
	}
	btOverlapFilterCallback* getOverlapFilterCallback()
	{
		return 0;
	}
	virtual void setOverlapFilterCallback(btOverlapFilterCallback* )
	{
	}

	virtual void processAllOverlappingPairs(btOverlapCallback*, btDispatcher* )
	{
	}

	virtual btBroadphasePair* findPair(btBroadphaseProxy* , btBroadphaseProxy* )
	{
		return 0;
	}

	virtual bool hasDeferredRemoval()
	{
		return true;
	}

	virtual void setInternalGhostPairCallback(btOverlappingPairCallback* )
	{
	}

	virtual btBroadphasePair* addOverlappingPair(btBroadphaseProxy* , btBroadphaseProxy* )
	{
		return 0;
	}

	virtual void* removeOverlappingPair(btBroadphaseProxy* , btBroadphaseProxy* , btDispatcher* )
	{
		return 0;
	}

	virtual void removeOverlappingPairsContainingProxy(btBroadphaseProxy* , btDispatcher* )
	{
	}

	virtual void sortOverlappingPairs(btDispatcher* dispatcher)
	{
		(void)dispatcher;
	}
};

#endif  
