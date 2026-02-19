

#ifndef BT_SIMPLE_BROADPHASE_H
#define BT_SIMPLE_BROADPHASE_H

#include "btOverlappingPairCache.h"

struct btSimpleBroadphaseProxy : public btBroadphaseProxy
{
	int m_nextFree;

	

	btSimpleBroadphaseProxy(){};

	btSimpleBroadphaseProxy(const btVector3& minpt, const btVector3& maxpt, int shapeType, void* userPtr, int collisionFilterGroup, int collisionFilterMask)
		: btBroadphaseProxy(minpt, maxpt, userPtr, collisionFilterGroup, collisionFilterMask)
	{
		(void)shapeType;
	}

	SIMD_FORCE_INLINE void SetNextFree(int next) { m_nextFree = next; }
	SIMD_FORCE_INLINE int GetNextFree() const { return m_nextFree; }
};



class btSimpleBroadphase : public btBroadphaseInterface
{
protected:
	int m_numHandles;  
	int m_maxHandles;  
	int m_LastHandleIndex;

	btSimpleBroadphaseProxy* m_pHandles;  

	void* m_pHandlesRawPtr;
	int m_firstFreeHandle;  

	int allocHandle()
	{
		btAssert(m_numHandles < m_maxHandles);
		int freeHandle = m_firstFreeHandle;
		m_firstFreeHandle = m_pHandles[freeHandle].GetNextFree();
		m_numHandles++;
		if (freeHandle > m_LastHandleIndex)
		{
			m_LastHandleIndex = freeHandle;
		}
		return freeHandle;
	}

	void freeHandle(btSimpleBroadphaseProxy* proxy)
	{
		int handle = int(proxy - m_pHandles);
		btAssert(handle >= 0 && handle < m_maxHandles);
		if (handle == m_LastHandleIndex)
		{
			m_LastHandleIndex--;
		}
		proxy->SetNextFree(m_firstFreeHandle);
		m_firstFreeHandle = handle;

		proxy->m_clientObject = 0;

		m_numHandles--;
	}

	btOverlappingPairCache* m_pairCache;
	bool m_ownsPairCache;

	int m_invalidPair;

	inline btSimpleBroadphaseProxy* getSimpleProxyFromProxy(btBroadphaseProxy* proxy)
	{
		btSimpleBroadphaseProxy* proxy0 = static_cast<btSimpleBroadphaseProxy*>(proxy);
		return proxy0;
	}

	inline const btSimpleBroadphaseProxy* getSimpleProxyFromProxy(btBroadphaseProxy* proxy) const
	{
		const btSimpleBroadphaseProxy* proxy0 = static_cast<const btSimpleBroadphaseProxy*>(proxy);
		return proxy0;
	}

	
	virtual void resetPool(btDispatcher* dispatcher);

	void validate();

protected:
public:
	btSimpleBroadphase(int maxProxies = 16384, btOverlappingPairCache* overlappingPairCache = 0);
	virtual ~btSimpleBroadphase();

	static bool aabbOverlap(btSimpleBroadphaseProxy* proxy0, btSimpleBroadphaseProxy* proxy1);

	virtual btBroadphaseProxy* createProxy(const btVector3& aabbMin, const btVector3& aabbMax, int shapeType, void* userPtr, int collisionFilterGroup, int collisionFilterMask, btDispatcher* dispatcher);

	virtual void calculateOverlappingPairs(btDispatcher* dispatcher);

	virtual void destroyProxy(btBroadphaseProxy* proxy, btDispatcher* dispatcher);
	virtual void setAabb(btBroadphaseProxy* proxy, const btVector3& aabbMin, const btVector3& aabbMax, btDispatcher* dispatcher);
	virtual void getAabb(btBroadphaseProxy* proxy, btVector3& aabbMin, btVector3& aabbMax) const;

	virtual void rayTest(const btVector3& rayFrom, const btVector3& rayTo, btBroadphaseRayCallback& rayCallback, const btVector3& aabbMin = btVector3(0, 0, 0), const btVector3& aabbMax = btVector3(0, 0, 0));
	virtual void aabbTest(const btVector3& aabbMin, const btVector3& aabbMax, btBroadphaseAabbCallback& callback);

	btOverlappingPairCache* getOverlappingPairCache()
	{
		return m_pairCache;
	}
	const btOverlappingPairCache* getOverlappingPairCache() const
	{
		return m_pairCache;
	}

	bool testAabbOverlap(btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1);

	
	
	virtual void getBroadphaseAabb(btVector3& aabbMin, btVector3& aabbMax) const
	{
		aabbMin.setValue(-BT_LARGE_FLOAT, -BT_LARGE_FLOAT, -BT_LARGE_FLOAT);
		aabbMax.setValue(BT_LARGE_FLOAT, BT_LARGE_FLOAT, BT_LARGE_FLOAT);
	}

	virtual void printStats()
	{
		
		
	}
};

#endif  
