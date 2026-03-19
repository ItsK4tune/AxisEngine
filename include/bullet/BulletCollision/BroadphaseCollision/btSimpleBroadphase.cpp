

#include "btSimpleBroadphase.h"
#include "BulletCollision/BroadphaseCollision/btDispatcher.h"
#include "BulletCollision/BroadphaseCollision/btCollisionAlgorithm.h"

#include "LinearMath/btVector3.h"
#include "LinearMath/btTransform.h"
#include "LinearMath/btMatrix3x3.h"
#include "LinearMath/btAabbUtil2.h"

#include <new>

void btSimpleBroadphase::validate()
{
	for (int i = 0; i < m_numHandles; i++)
	{
		for (int j = i + 1; j < m_numHandles; j++)
		{
			btAssert(&m_pHandles[i] != &m_pHandles[j]);
		}
	}
}

btSimpleBroadphase::btSimpleBroadphase(int maxProxies, btOverlappingPairCache* overlappingPairCache)
	: m_pairCache(overlappingPairCache),
	  m_ownsPairCache(false),
	  m_invalidPair(0)
{
	if (!overlappingPairCache)
	{
		void* mem = btAlignedAlloc(sizeof(btHashedOverlappingPairCache), 16);
		m_pairCache = new (mem) btHashedOverlappingPairCache();
		m_ownsPairCache = true;
	}

	
	m_pHandlesRawPtr = btAlignedAlloc(sizeof(btSimpleBroadphaseProxy) * maxProxies, 16);
	m_pHandles = new (m_pHandlesRawPtr) btSimpleBroadphaseProxy[maxProxies];
	m_maxHandles = maxProxies;
	m_numHandles = 0;
	m_firstFreeHandle = 0;
	m_LastHandleIndex = -1;

	{
		for (int i = m_firstFreeHandle; i < maxProxies; i++)
		{
			m_pHandles[i].SetNextFree(i + 1);
			m_pHandles[i].m_uniqueId = i + 2;  
		}
		m_pHandles[maxProxies - 1].SetNextFree(0);
	}
}

btSimpleBroadphase::~btSimpleBroadphase()
{
	btAlignedFree(m_pHandlesRawPtr);

	if (m_ownsPairCache)
	{
		m_pairCache->~btOverlappingPairCache();
		btAlignedFree(m_pairCache);
	}
}

btBroadphaseProxy* btSimpleBroadphase::createProxy(const btVector3& aabbMin, const btVector3& aabbMax, int shapeType, void* userPtr, int collisionFilterGroup, int collisionFilterMask, btDispatcher* )
{
	if (m_numHandles >= m_maxHandles)
	{
		btAssert(0);
		return 0;  
	}
	btAssert(aabbMin[0] <= aabbMax[0] && aabbMin[1] <= aabbMax[1] && aabbMin[2] <= aabbMax[2]);

	int newHandleIndex = allocHandle();
	btSimpleBroadphaseProxy* proxy = new (&m_pHandles[newHandleIndex]) btSimpleBroadphaseProxy(aabbMin, aabbMax, shapeType, userPtr, collisionFilterGroup, collisionFilterMask);

	return proxy;
}

class RemovingOverlapCallback : public btOverlapCallback
{
protected:
	virtual bool processOverlap(btBroadphasePair& pair)
	{
		(void)pair;
		btAssert(0);
		return false;
	}
};

class RemovePairContainingProxy
{
	btBroadphaseProxy* m_targetProxy;

public:
	virtual ~RemovePairContainingProxy()
	{
	}

protected:
	virtual bool processOverlap(btBroadphasePair& pair)
	{
		btSimpleBroadphaseProxy* proxy0 = static_cast<btSimpleBroadphaseProxy*>(pair.m_pProxy0);
		btSimpleBroadphaseProxy* proxy1 = static_cast<btSimpleBroadphaseProxy*>(pair.m_pProxy1);

		return ((m_targetProxy == proxy0 || m_targetProxy == proxy1));
	};
};

void btSimpleBroadphase::destroyProxy(btBroadphaseProxy* proxyOrg, btDispatcher* dispatcher)
{
	m_pairCache->removeOverlappingPairsContainingProxy(proxyOrg, dispatcher);

	btSimpleBroadphaseProxy* proxy0 = static_cast<btSimpleBroadphaseProxy*>(proxyOrg);
	freeHandle(proxy0);

	
}

void btSimpleBroadphase::getAabb(btBroadphaseProxy* proxy, btVector3& aabbMin, btVector3& aabbMax) const
{
	const btSimpleBroadphaseProxy* sbp = getSimpleProxyFromProxy(proxy);
	aabbMin = sbp->m_aabbMin;
	aabbMax = sbp->m_aabbMax;
}

void btSimpleBroadphase::setAabb(btBroadphaseProxy* proxy, const btVector3& aabbMin, const btVector3& aabbMax, btDispatcher* )
{
	btSimpleBroadphaseProxy* sbp = getSimpleProxyFromProxy(proxy);
	sbp->m_aabbMin = aabbMin;
	sbp->m_aabbMax = aabbMax;
}

void btSimpleBroadphase::rayTest(const btVector3& rayFrom, const btVector3& rayTo, btBroadphaseRayCallback& rayCallback, const btVector3& aabbMin, const btVector3& aabbMax)
{
	for (int i = 0; i <= m_LastHandleIndex; i++)
	{
		btSimpleBroadphaseProxy* proxy = &m_pHandles[i];
		if (!proxy->m_clientObject)
		{
			continue;
		}
		rayCallback.process(proxy);
	}
}

void btSimpleBroadphase::aabbTest(const btVector3& aabbMin, const btVector3& aabbMax, btBroadphaseAabbCallback& callback)
{
	for (int i = 0; i <= m_LastHandleIndex; i++)
	{
		btSimpleBroadphaseProxy* proxy = &m_pHandles[i];
		if (!proxy->m_clientObject)
		{
			continue;
		}
		if (TestAabbAgainstAabb2(aabbMin, aabbMax, proxy->m_aabbMin, proxy->m_aabbMax))
		{
			callback.process(proxy);
		}
	}
}

bool btSimpleBroadphase::aabbOverlap(btSimpleBroadphaseProxy* proxy0, btSimpleBroadphaseProxy* proxy1)
{
	return proxy0->m_aabbMin[0] <= proxy1->m_aabbMax[0] && proxy1->m_aabbMin[0] <= proxy0->m_aabbMax[0] &&
		   proxy0->m_aabbMin[1] <= proxy1->m_aabbMax[1] && proxy1->m_aabbMin[1] <= proxy0->m_aabbMax[1] &&
		   proxy0->m_aabbMin[2] <= proxy1->m_aabbMax[2] && proxy1->m_aabbMin[2] <= proxy0->m_aabbMax[2];
}


class CheckOverlapCallback : public btOverlapCallback
{
public:
	virtual bool processOverlap(btBroadphasePair& pair)
	{
		return (!btSimpleBroadphase::aabbOverlap(static_cast<btSimpleBroadphaseProxy*>(pair.m_pProxy0), static_cast<btSimpleBroadphaseProxy*>(pair.m_pProxy1)));
	}
};

void btSimpleBroadphase::calculateOverlappingPairs(btDispatcher* dispatcher)
{
	
	int i, j;
	if (m_numHandles >= 0)
	{
		int new_largest_index = -1;
		for (i = 0; i <= m_LastHandleIndex; i++)
		{
			btSimpleBroadphaseProxy* proxy0 = &m_pHandles[i];
			if (!proxy0->m_clientObject)
			{
				continue;
			}
			new_largest_index = i;
			for (j = i + 1; j <= m_LastHandleIndex; j++)
			{
				btSimpleBroadphaseProxy* proxy1 = &m_pHandles[j];
				btAssert(proxy0 != proxy1);
				if (!proxy1->m_clientObject)
				{
					continue;
				}

				btSimpleBroadphaseProxy* p0 = getSimpleProxyFromProxy(proxy0);
				btSimpleBroadphaseProxy* p1 = getSimpleProxyFromProxy(proxy1);

				if (aabbOverlap(p0, p1))
				{
					if (!m_pairCache->findPair(proxy0, proxy1))
					{
						m_pairCache->addOverlappingPair(proxy0, proxy1);
					}
				}
				else
				{
					if (!m_pairCache->hasDeferredRemoval())
					{
						if (m_pairCache->findPair(proxy0, proxy1))
						{
							m_pairCache->removeOverlappingPair(proxy0, proxy1, dispatcher);
						}
					}
				}
			}
		}

		m_LastHandleIndex = new_largest_index;

		if (m_ownsPairCache && m_pairCache->hasDeferredRemoval())
		{
			btBroadphasePairArray& overlappingPairArray = m_pairCache->getOverlappingPairArray();

			
			overlappingPairArray.quickSort(btBroadphasePairSortPredicate());

			overlappingPairArray.resize(overlappingPairArray.size() - m_invalidPair);
			m_invalidPair = 0;

			btBroadphasePair previousPair;
			previousPair.m_pProxy0 = 0;
			previousPair.m_pProxy1 = 0;
			previousPair.m_algorithm = 0;

			for (i = 0; i < overlappingPairArray.size(); i++)
			{
				btBroadphasePair& pair = overlappingPairArray[i];

				bool isDuplicate = (pair == previousPair);

				previousPair = pair;

				bool needsRemoval = false;

				if (!isDuplicate)
				{
					bool hasOverlap = testAabbOverlap(pair.m_pProxy0, pair.m_pProxy1);

					if (hasOverlap)
					{
						needsRemoval = false;  
					}
					else
					{
						needsRemoval = true;
					}
				}
				else
				{
					
					needsRemoval = true;
					
					btAssert(!pair.m_algorithm);
				}

				if (needsRemoval)
				{
					m_pairCache->cleanOverlappingPair(pair, dispatcher);

					
					
					pair.m_pProxy0 = 0;
					pair.m_pProxy1 = 0;
					m_invalidPair++;
				}
			}

			
#define CLEAN_INVALID_PAIRS 1
#ifdef CLEAN_INVALID_PAIRS

			
			overlappingPairArray.quickSort(btBroadphasePairSortPredicate());

			overlappingPairArray.resize(overlappingPairArray.size() - m_invalidPair);
			m_invalidPair = 0;
#endif  
		}
	}
}

bool btSimpleBroadphase::testAabbOverlap(btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1)
{
	btSimpleBroadphaseProxy* p0 = getSimpleProxyFromProxy(proxy0);
	btSimpleBroadphaseProxy* p1 = getSimpleProxyFromProxy(proxy1);
	return aabbOverlap(p0, p1);
}

void btSimpleBroadphase::resetPool(btDispatcher* dispatcher)
{
	
}
