

















#ifndef BT_AXIS_SWEEP_3_INTERNAL_H
#define BT_AXIS_SWEEP_3_INTERNAL_H

#include "LinearMath/btVector3.h"
#include "btOverlappingPairCache.h"
#include "btBroadphaseInterface.h"
#include "btBroadphaseProxy.h"
#include "btOverlappingPairCallback.h"
#include "btDbvtBroadphase.h"


#define USE_OVERLAP_TEST_ON_REMOVES 1




template <typename BP_FP_INT_TYPE>
class btAxisSweep3Internal : public btBroadphaseInterface
{
protected:
	BP_FP_INT_TYPE m_bpHandleMask;
	BP_FP_INT_TYPE m_handleSentinel;

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	class Edge
	{
	public:
		BP_FP_INT_TYPE m_pos;  
		BP_FP_INT_TYPE m_handle;

		BP_FP_INT_TYPE IsMax() const { return static_cast<BP_FP_INT_TYPE>(m_pos & 1); }
	};

public:
	class Handle : public btBroadphaseProxy
	{
	public:
		BT_DECLARE_ALIGNED_ALLOCATOR();

		
		BP_FP_INT_TYPE m_minEdges[3], m_maxEdges[3];  
													  
		btBroadphaseProxy* m_dbvtProxy;               
		

		SIMD_FORCE_INLINE void SetNextFree(BP_FP_INT_TYPE next) { m_minEdges[0] = next; }
		SIMD_FORCE_INLINE BP_FP_INT_TYPE GetNextFree() const { return m_minEdges[0]; }
	};  

protected:
	btVector3 m_worldAabbMin;  
	btVector3 m_worldAabbMax;  

	btVector3 m_quantize;  

	BP_FP_INT_TYPE m_numHandles;  
	BP_FP_INT_TYPE m_maxHandles;  
	Handle* m_pHandles;           

	BP_FP_INT_TYPE m_firstFreeHandle;  

	Edge* m_pEdges[3];  
	void* m_pEdgesRawPtr[3];

	btOverlappingPairCache* m_pairCache;

	
	btOverlappingPairCallback* m_userPairCallback;

	bool m_ownsPairCache;

	int m_invalidPair;

	
	
	btDbvtBroadphase* m_raycastAccelerator;
	btOverlappingPairCache* m_nullPairCache;

	
	BP_FP_INT_TYPE allocHandle();
	void freeHandle(BP_FP_INT_TYPE handle);

	bool testOverlap2D(const Handle* pHandleA, const Handle* pHandleB, int axis0, int axis1);

#ifdef DEBUG_BROADPHASE
	void debugPrintAxis(int axis, bool checkCardinality = true);
#endif  

	
	

	void sortMinDown(int axis, BP_FP_INT_TYPE edge, btDispatcher* dispatcher, bool updateOverlaps);
	void sortMinUp(int axis, BP_FP_INT_TYPE edge, btDispatcher* dispatcher, bool updateOverlaps);
	void sortMaxDown(int axis, BP_FP_INT_TYPE edge, btDispatcher* dispatcher, bool updateOverlaps);
	void sortMaxUp(int axis, BP_FP_INT_TYPE edge, btDispatcher* dispatcher, bool updateOverlaps);

public:
	btAxisSweep3Internal(const btVector3& worldAabbMin, const btVector3& worldAabbMax, BP_FP_INT_TYPE handleMask, BP_FP_INT_TYPE handleSentinel, BP_FP_INT_TYPE maxHandles = 16384, btOverlappingPairCache* pairCache = 0, bool disableRaycastAccelerator = false);

	virtual ~btAxisSweep3Internal();

	BP_FP_INT_TYPE getNumHandles() const
	{
		return m_numHandles;
	}

	virtual void calculateOverlappingPairs(btDispatcher* dispatcher);

	BP_FP_INT_TYPE addHandle(const btVector3& aabbMin, const btVector3& aabbMax, void* pOwner, int collisionFilterGroup, int collisionFilterMask, btDispatcher* dispatcher);
	void removeHandle(BP_FP_INT_TYPE handle, btDispatcher* dispatcher);
	void updateHandle(BP_FP_INT_TYPE handle, const btVector3& aabbMin, const btVector3& aabbMax, btDispatcher* dispatcher);
	SIMD_FORCE_INLINE Handle* getHandle(BP_FP_INT_TYPE index) const { return m_pHandles + index; }

	virtual void resetPool(btDispatcher* dispatcher);

	void processAllOverlappingPairs(btOverlapCallback* callback);

	
	virtual btBroadphaseProxy* createProxy(const btVector3& aabbMin, const btVector3& aabbMax, int shapeType, void* userPtr, int collisionFilterGroup, int collisionFilterMask, btDispatcher* dispatcher);
	virtual void destroyProxy(btBroadphaseProxy* proxy, btDispatcher* dispatcher);
	virtual void setAabb(btBroadphaseProxy* proxy, const btVector3& aabbMin, const btVector3& aabbMax, btDispatcher* dispatcher);
	virtual void getAabb(btBroadphaseProxy* proxy, btVector3& aabbMin, btVector3& aabbMax) const;

	virtual void rayTest(const btVector3& rayFrom, const btVector3& rayTo, btBroadphaseRayCallback& rayCallback, const btVector3& aabbMin = btVector3(0, 0, 0), const btVector3& aabbMax = btVector3(0, 0, 0));
	virtual void aabbTest(const btVector3& aabbMin, const btVector3& aabbMax, btBroadphaseAabbCallback& callback);

	void quantize(BP_FP_INT_TYPE* out, const btVector3& point, int isMax) const;
	
	void unQuantize(btBroadphaseProxy* proxy, btVector3& aabbMin, btVector3& aabbMax) const;

	bool testAabbOverlap(btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1);

	btOverlappingPairCache* getOverlappingPairCache()
	{
		return m_pairCache;
	}
	const btOverlappingPairCache* getOverlappingPairCache() const
	{
		return m_pairCache;
	}

	void setOverlappingPairUserCallback(btOverlappingPairCallback* pairCallback)
	{
		m_userPairCallback = pairCallback;
	}
	const btOverlappingPairCallback* getOverlappingPairUserCallback() const
	{
		return m_userPairCallback;
	}

	
	
	virtual void getBroadphaseAabb(btVector3& aabbMin, btVector3& aabbMax) const
	{
		aabbMin = m_worldAabbMin;
		aabbMax = m_worldAabbMax;
	}

	virtual void printStats()
	{
		
	}
};



#ifdef DEBUG_BROADPHASE
#include <stdio.h>

template <typename BP_FP_INT_TYPE>
void btAxisSweep3<BP_FP_INT_TYPE>::debugPrintAxis(int axis, bool checkCardinality)
{
	int numEdges = m_pHandles[0].m_maxEdges[axis];
	printf("SAP Axis %d, numEdges=%d\n", axis, numEdges);

	int i;
	for (i = 0; i < numEdges + 1; i++)
	{
		Edge* pEdge = m_pEdges[axis] + i;
		Handle* pHandlePrev = getHandle(pEdge->m_handle);
		int handleIndex = pEdge->IsMax() ? pHandlePrev->m_maxEdges[axis] : pHandlePrev->m_minEdges[axis];
		char beginOrEnd;
		beginOrEnd = pEdge->IsMax() ? 'E' : 'B';
		printf("	[%c,h=%d,p=%x,i=%d]\n", beginOrEnd, pEdge->m_handle, pEdge->m_pos, handleIndex);
	}

	if (checkCardinality)
		btAssert(numEdges == m_numHandles * 2 + 1);
}
#endif  

template <typename BP_FP_INT_TYPE>
btBroadphaseProxy* btAxisSweep3Internal<BP_FP_INT_TYPE>::createProxy(const btVector3& aabbMin, const btVector3& aabbMax, int shapeType, void* userPtr, int collisionFilterGroup, int collisionFilterMask, btDispatcher* dispatcher)
{
	(void)shapeType;
	BP_FP_INT_TYPE handleId = addHandle(aabbMin, aabbMax, userPtr, collisionFilterGroup, collisionFilterMask, dispatcher);

	Handle* handle = getHandle(handleId);

	if (m_raycastAccelerator)
	{
		btBroadphaseProxy* rayProxy = m_raycastAccelerator->createProxy(aabbMin, aabbMax, shapeType, userPtr, collisionFilterGroup, collisionFilterMask, dispatcher);
		handle->m_dbvtProxy = rayProxy;
	}
	return handle;
}

template <typename BP_FP_INT_TYPE>
void btAxisSweep3Internal<BP_FP_INT_TYPE>::destroyProxy(btBroadphaseProxy* proxy, btDispatcher* dispatcher)
{
	Handle* handle = static_cast<Handle*>(proxy);
	if (m_raycastAccelerator)
		m_raycastAccelerator->destroyProxy(handle->m_dbvtProxy, dispatcher);
	removeHandle(static_cast<BP_FP_INT_TYPE>(handle->m_uniqueId), dispatcher);
}

template <typename BP_FP_INT_TYPE>
void btAxisSweep3Internal<BP_FP_INT_TYPE>::setAabb(btBroadphaseProxy* proxy, const btVector3& aabbMin, const btVector3& aabbMax, btDispatcher* dispatcher)
{
	Handle* handle = static_cast<Handle*>(proxy);
	handle->m_aabbMin = aabbMin;
	handle->m_aabbMax = aabbMax;
	updateHandle(static_cast<BP_FP_INT_TYPE>(handle->m_uniqueId), aabbMin, aabbMax, dispatcher);
	if (m_raycastAccelerator)
		m_raycastAccelerator->setAabb(handle->m_dbvtProxy, aabbMin, aabbMax, dispatcher);
}

template <typename BP_FP_INT_TYPE>
void btAxisSweep3Internal<BP_FP_INT_TYPE>::rayTest(const btVector3& rayFrom, const btVector3& rayTo, btBroadphaseRayCallback& rayCallback, const btVector3& aabbMin, const btVector3& aabbMax)
{
	if (m_raycastAccelerator)
	{
		m_raycastAccelerator->rayTest(rayFrom, rayTo, rayCallback, aabbMin, aabbMax);
	}
	else
	{
		
		BP_FP_INT_TYPE axis = 0;
		
		for (BP_FP_INT_TYPE i = 1; i < m_numHandles * 2 + 1; i++)
		{
			if (m_pEdges[axis][i].IsMax())
			{
				rayCallback.process(getHandle(m_pEdges[axis][i].m_handle));
			}
		}
	}
}

template <typename BP_FP_INT_TYPE>
void btAxisSweep3Internal<BP_FP_INT_TYPE>::aabbTest(const btVector3& aabbMin, const btVector3& aabbMax, btBroadphaseAabbCallback& callback)
{
	if (m_raycastAccelerator)
	{
		m_raycastAccelerator->aabbTest(aabbMin, aabbMax, callback);
	}
	else
	{
		
		BP_FP_INT_TYPE axis = 0;
		
		for (BP_FP_INT_TYPE i = 1; i < m_numHandles * 2 + 1; i++)
		{
			if (m_pEdges[axis][i].IsMax())
			{
				Handle* handle = getHandle(m_pEdges[axis][i].m_handle);
				if (TestAabbAgainstAabb2(aabbMin, aabbMax, handle->m_aabbMin, handle->m_aabbMax))
				{
					callback.process(handle);
				}
			}
		}
	}
}

template <typename BP_FP_INT_TYPE>
void btAxisSweep3Internal<BP_FP_INT_TYPE>::getAabb(btBroadphaseProxy* proxy, btVector3& aabbMin, btVector3& aabbMax) const
{
	Handle* pHandle = static_cast<Handle*>(proxy);
	aabbMin = pHandle->m_aabbMin;
	aabbMax = pHandle->m_aabbMax;
}

template <typename BP_FP_INT_TYPE>
void btAxisSweep3Internal<BP_FP_INT_TYPE>::unQuantize(btBroadphaseProxy* proxy, btVector3& aabbMin, btVector3& aabbMax) const
{
	Handle* pHandle = static_cast<Handle*>(proxy);

	unsigned short vecInMin[3];
	unsigned short vecInMax[3];

	vecInMin[0] = m_pEdges[0][pHandle->m_minEdges[0]].m_pos;
	vecInMax[0] = m_pEdges[0][pHandle->m_maxEdges[0]].m_pos + 1;
	vecInMin[1] = m_pEdges[1][pHandle->m_minEdges[1]].m_pos;
	vecInMax[1] = m_pEdges[1][pHandle->m_maxEdges[1]].m_pos + 1;
	vecInMin[2] = m_pEdges[2][pHandle->m_minEdges[2]].m_pos;
	vecInMax[2] = m_pEdges[2][pHandle->m_maxEdges[2]].m_pos + 1;

	aabbMin.setValue((btScalar)(vecInMin[0]) / (m_quantize.getX()), (btScalar)(vecInMin[1]) / (m_quantize.getY()), (btScalar)(vecInMin[2]) / (m_quantize.getZ()));
	aabbMin += m_worldAabbMin;

	aabbMax.setValue((btScalar)(vecInMax[0]) / (m_quantize.getX()), (btScalar)(vecInMax[1]) / (m_quantize.getY()), (btScalar)(vecInMax[2]) / (m_quantize.getZ()));
	aabbMax += m_worldAabbMin;
}

template <typename BP_FP_INT_TYPE>
btAxisSweep3Internal<BP_FP_INT_TYPE>::btAxisSweep3Internal(const btVector3& worldAabbMin, const btVector3& worldAabbMax, BP_FP_INT_TYPE handleMask, BP_FP_INT_TYPE handleSentinel, BP_FP_INT_TYPE userMaxHandles, btOverlappingPairCache* pairCache, bool disableRaycastAccelerator)
	: m_bpHandleMask(handleMask),
	  m_handleSentinel(handleSentinel),
	  m_pairCache(pairCache),
	  m_userPairCallback(0),
	  m_ownsPairCache(false),
	  m_invalidPair(0),
	  m_raycastAccelerator(0)
{
	BP_FP_INT_TYPE maxHandles = static_cast<BP_FP_INT_TYPE>(userMaxHandles + 1);  

	if (!m_pairCache)
	{
		void* ptr = btAlignedAlloc(sizeof(btHashedOverlappingPairCache), 16);
		m_pairCache = new (ptr) btHashedOverlappingPairCache();
		m_ownsPairCache = true;
	}

	if (!disableRaycastAccelerator)
	{
		m_nullPairCache = new (btAlignedAlloc(sizeof(btNullPairCache), 16)) btNullPairCache();
		m_raycastAccelerator = new (btAlignedAlloc(sizeof(btDbvtBroadphase), 16)) btDbvtBroadphase(m_nullPairCache);  
		m_raycastAccelerator->m_deferedcollide = true;                                                                
	}

	

	
	m_worldAabbMin = worldAabbMin;
	m_worldAabbMax = worldAabbMax;

	btVector3 aabbSize = m_worldAabbMax - m_worldAabbMin;

	BP_FP_INT_TYPE maxInt = m_handleSentinel;

	m_quantize = btVector3(btScalar(maxInt), btScalar(maxInt), btScalar(maxInt)) / aabbSize;

	
	m_pHandles = new Handle[maxHandles];

	m_maxHandles = maxHandles;
	m_numHandles = 0;

	
	m_firstFreeHandle = 1;
	{
		for (BP_FP_INT_TYPE i = m_firstFreeHandle; i < maxHandles; i++)
			m_pHandles[i].SetNextFree(static_cast<BP_FP_INT_TYPE>(i + 1));
		m_pHandles[maxHandles - 1].SetNextFree(0);
	}

	{
		
		for (int i = 0; i < 3; i++)
		{
			m_pEdgesRawPtr[i] = btAlignedAlloc(sizeof(Edge) * maxHandles * 2, 16);
			m_pEdges[i] = new (m_pEdgesRawPtr[i]) Edge[maxHandles * 2];
		}
	}
	

	

	m_pHandles[0].m_clientObject = 0;

	for (int axis = 0; axis < 3; axis++)
	{
		m_pHandles[0].m_minEdges[axis] = 0;
		m_pHandles[0].m_maxEdges[axis] = 1;

		m_pEdges[axis][0].m_pos = 0;
		m_pEdges[axis][0].m_handle = 0;
		m_pEdges[axis][1].m_pos = m_handleSentinel;
		m_pEdges[axis][1].m_handle = 0;
#ifdef DEBUG_BROADPHASE
		debugPrintAxis(axis);
#endif  
	}
}

template <typename BP_FP_INT_TYPE>
btAxisSweep3Internal<BP_FP_INT_TYPE>::~btAxisSweep3Internal()
{
	if (m_raycastAccelerator)
	{
		m_nullPairCache->~btOverlappingPairCache();
		btAlignedFree(m_nullPairCache);
		m_raycastAccelerator->~btDbvtBroadphase();
		btAlignedFree(m_raycastAccelerator);
	}

	for (int i = 2; i >= 0; i--)
	{
		btAlignedFree(m_pEdgesRawPtr[i]);
	}
	delete[] m_pHandles;

	if (m_ownsPairCache)
	{
		m_pairCache->~btOverlappingPairCache();
		btAlignedFree(m_pairCache);
	}
}

template <typename BP_FP_INT_TYPE>
void btAxisSweep3Internal<BP_FP_INT_TYPE>::quantize(BP_FP_INT_TYPE* out, const btVector3& point, int isMax) const
{
#ifdef OLD_CLAMPING_METHOD
	
	
	btVector3 clampedPoint(point);
	clampedPoint.setMax(m_worldAabbMin);
	clampedPoint.setMin(m_worldAabbMax);
	btVector3 v = (clampedPoint - m_worldAabbMin) * m_quantize;
	out[0] = (BP_FP_INT_TYPE)(((BP_FP_INT_TYPE)v.getX() & m_bpHandleMask) | isMax);
	out[1] = (BP_FP_INT_TYPE)(((BP_FP_INT_TYPE)v.getY() & m_bpHandleMask) | isMax);
	out[2] = (BP_FP_INT_TYPE)(((BP_FP_INT_TYPE)v.getZ() & m_bpHandleMask) | isMax);
#else
	btVector3 v = (point - m_worldAabbMin) * m_quantize;
	out[0] = (v[0] <= 0) ? (BP_FP_INT_TYPE)isMax : (v[0] >= m_handleSentinel) ? (BP_FP_INT_TYPE)((m_handleSentinel & m_bpHandleMask) | isMax) : (BP_FP_INT_TYPE)(((BP_FP_INT_TYPE)v[0] & m_bpHandleMask) | isMax);
	out[1] = (v[1] <= 0) ? (BP_FP_INT_TYPE)isMax : (v[1] >= m_handleSentinel) ? (BP_FP_INT_TYPE)((m_handleSentinel & m_bpHandleMask) | isMax) : (BP_FP_INT_TYPE)(((BP_FP_INT_TYPE)v[1] & m_bpHandleMask) | isMax);
	out[2] = (v[2] <= 0) ? (BP_FP_INT_TYPE)isMax : (v[2] >= m_handleSentinel) ? (BP_FP_INT_TYPE)((m_handleSentinel & m_bpHandleMask) | isMax) : (BP_FP_INT_TYPE)(((BP_FP_INT_TYPE)v[2] & m_bpHandleMask) | isMax);
#endif  
}

template <typename BP_FP_INT_TYPE>
BP_FP_INT_TYPE btAxisSweep3Internal<BP_FP_INT_TYPE>::allocHandle()
{
	btAssert(m_firstFreeHandle);

	BP_FP_INT_TYPE handle = m_firstFreeHandle;
	m_firstFreeHandle = getHandle(handle)->GetNextFree();
	m_numHandles++;

	return handle;
}

template <typename BP_FP_INT_TYPE>
void btAxisSweep3Internal<BP_FP_INT_TYPE>::freeHandle(BP_FP_INT_TYPE handle)
{
	btAssert(handle > 0 && handle < m_maxHandles);

	getHandle(handle)->SetNextFree(m_firstFreeHandle);
	m_firstFreeHandle = handle;

	m_numHandles--;
}

template <typename BP_FP_INT_TYPE>
BP_FP_INT_TYPE btAxisSweep3Internal<BP_FP_INT_TYPE>::addHandle(const btVector3& aabbMin, const btVector3& aabbMax, void* pOwner, int collisionFilterGroup, int collisionFilterMask, btDispatcher* dispatcher)
{
	
	BP_FP_INT_TYPE min[3], max[3];
	quantize(min, aabbMin, 0);
	quantize(max, aabbMax, 1);

	
	BP_FP_INT_TYPE handle = allocHandle();

	Handle* pHandle = getHandle(handle);

	pHandle->m_uniqueId = static_cast<int>(handle);
	
	pHandle->m_clientObject = pOwner;
	pHandle->m_collisionFilterGroup = collisionFilterGroup;
	pHandle->m_collisionFilterMask = collisionFilterMask;

	
	BP_FP_INT_TYPE limit = static_cast<BP_FP_INT_TYPE>(m_numHandles * 2);

	
	for (BP_FP_INT_TYPE axis = 0; axis < 3; axis++)
	{
		m_pHandles[0].m_maxEdges[axis] += 2;

		m_pEdges[axis][limit + 1] = m_pEdges[axis][limit - 1];

		m_pEdges[axis][limit - 1].m_pos = min[axis];
		m_pEdges[axis][limit - 1].m_handle = handle;

		m_pEdges[axis][limit].m_pos = max[axis];
		m_pEdges[axis][limit].m_handle = handle;

		pHandle->m_minEdges[axis] = static_cast<BP_FP_INT_TYPE>(limit - 1);
		pHandle->m_maxEdges[axis] = limit;
	}

	
	sortMinDown(0, pHandle->m_minEdges[0], dispatcher, false);
	sortMaxDown(0, pHandle->m_maxEdges[0], dispatcher, false);
	sortMinDown(1, pHandle->m_minEdges[1], dispatcher, false);
	sortMaxDown(1, pHandle->m_maxEdges[1], dispatcher, false);
	sortMinDown(2, pHandle->m_minEdges[2], dispatcher, true);
	sortMaxDown(2, pHandle->m_maxEdges[2], dispatcher, true);

	return handle;
}

template <typename BP_FP_INT_TYPE>
void btAxisSweep3Internal<BP_FP_INT_TYPE>::removeHandle(BP_FP_INT_TYPE handle, btDispatcher* dispatcher)
{
	Handle* pHandle = getHandle(handle);

	
	
	
	if (!m_pairCache->hasDeferredRemoval())
	{
		m_pairCache->removeOverlappingPairsContainingProxy(pHandle, dispatcher);
	}

	
	int limit = static_cast<int>(m_numHandles * 2);

	int axis;

	for (axis = 0; axis < 3; axis++)
	{
		m_pHandles[0].m_maxEdges[axis] -= 2;
	}

	
	for (axis = 0; axis < 3; axis++)
	{
		Edge* pEdges = m_pEdges[axis];
		BP_FP_INT_TYPE max = pHandle->m_maxEdges[axis];
		pEdges[max].m_pos = m_handleSentinel;

		sortMaxUp(axis, max, dispatcher, false);

		BP_FP_INT_TYPE i = pHandle->m_minEdges[axis];
		pEdges[i].m_pos = m_handleSentinel;

		sortMinUp(axis, i, dispatcher, false);

		pEdges[limit - 1].m_handle = 0;
		pEdges[limit - 1].m_pos = m_handleSentinel;

#ifdef DEBUG_BROADPHASE
		debugPrintAxis(axis, false);
#endif  
	}

	
	freeHandle(handle);
}

template <typename BP_FP_INT_TYPE>
void btAxisSweep3Internal<BP_FP_INT_TYPE>::resetPool(btDispatcher* )
{
	if (m_numHandles == 0)
	{
		m_firstFreeHandle = 1;
		{
			for (BP_FP_INT_TYPE i = m_firstFreeHandle; i < m_maxHandles; i++)
				m_pHandles[i].SetNextFree(static_cast<BP_FP_INT_TYPE>(i + 1));
			m_pHandles[m_maxHandles - 1].SetNextFree(0);
		}
	}
}



template <typename BP_FP_INT_TYPE>
void btAxisSweep3Internal<BP_FP_INT_TYPE>::calculateOverlappingPairs(btDispatcher* dispatcher)
{
	if (m_pairCache->hasDeferredRemoval())
	{
		btBroadphasePairArray& overlappingPairArray = m_pairCache->getOverlappingPairArray();

		
		overlappingPairArray.quickSort(btBroadphasePairSortPredicate());

		overlappingPairArray.resize(overlappingPairArray.size() - m_invalidPair);
		m_invalidPair = 0;

		int i;

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

template <typename BP_FP_INT_TYPE>
bool btAxisSweep3Internal<BP_FP_INT_TYPE>::testAabbOverlap(btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1)
{
	const Handle* pHandleA = static_cast<Handle*>(proxy0);
	const Handle* pHandleB = static_cast<Handle*>(proxy1);

	

	for (int axis = 0; axis < 3; axis++)
	{
		if (pHandleA->m_maxEdges[axis] < pHandleB->m_minEdges[axis] ||
			pHandleB->m_maxEdges[axis] < pHandleA->m_minEdges[axis])
		{
			return false;
		}
	}
	return true;
}

template <typename BP_FP_INT_TYPE>
bool btAxisSweep3Internal<BP_FP_INT_TYPE>::testOverlap2D(const Handle* pHandleA, const Handle* pHandleB, int axis0, int axis1)
{
	

	if (pHandleA->m_maxEdges[axis0] < pHandleB->m_minEdges[axis0] ||
		pHandleB->m_maxEdges[axis0] < pHandleA->m_minEdges[axis0] ||
		pHandleA->m_maxEdges[axis1] < pHandleB->m_minEdges[axis1] ||
		pHandleB->m_maxEdges[axis1] < pHandleA->m_minEdges[axis1])
	{
		return false;
	}
	return true;
}

template <typename BP_FP_INT_TYPE>
void btAxisSweep3Internal<BP_FP_INT_TYPE>::updateHandle(BP_FP_INT_TYPE handle, const btVector3& aabbMin, const btVector3& aabbMax, btDispatcher* dispatcher)
{
	
	

	Handle* pHandle = getHandle(handle);

	
	BP_FP_INT_TYPE min[3], max[3];
	quantize(min, aabbMin, 0);
	quantize(max, aabbMax, 1);

	
	for (int axis = 0; axis < 3; axis++)
	{
		BP_FP_INT_TYPE emin = pHandle->m_minEdges[axis];
		BP_FP_INT_TYPE emax = pHandle->m_maxEdges[axis];

		int dmin = (int)min[axis] - (int)m_pEdges[axis][emin].m_pos;
		int dmax = (int)max[axis] - (int)m_pEdges[axis][emax].m_pos;

		m_pEdges[axis][emin].m_pos = min[axis];
		m_pEdges[axis][emax].m_pos = max[axis];

		
		if (dmin < 0)
			sortMinDown(axis, emin, dispatcher, true);

		if (dmax > 0)
			sortMaxUp(axis, emax, dispatcher, true);

		
		if (dmin > 0)
			sortMinUp(axis, emin, dispatcher, true);

		if (dmax < 0)
			sortMaxDown(axis, emax, dispatcher, true);

#ifdef DEBUG_BROADPHASE
		debugPrintAxis(axis);
#endif  
	}
}


template <typename BP_FP_INT_TYPE>
void btAxisSweep3Internal<BP_FP_INT_TYPE>::sortMinDown(int axis, BP_FP_INT_TYPE edge, btDispatcher* , bool updateOverlaps)
{
	Edge* pEdge = m_pEdges[axis] + edge;
	Edge* pPrev = pEdge - 1;
	Handle* pHandleEdge = getHandle(pEdge->m_handle);

	while (pEdge->m_pos < pPrev->m_pos)
	{
		Handle* pHandlePrev = getHandle(pPrev->m_handle);

		if (pPrev->IsMax())
		{
			
			const int axis1 = (1 << axis) & 3;
			const int axis2 = (1 << axis1) & 3;
			if (updateOverlaps && testOverlap2D(pHandleEdge, pHandlePrev, axis1, axis2))
			{
				m_pairCache->addOverlappingPair(pHandleEdge, pHandlePrev);
				if (m_userPairCallback)
					m_userPairCallback->addOverlappingPair(pHandleEdge, pHandlePrev);

				
			}

			
			pHandlePrev->m_maxEdges[axis]++;
		}
		else
			pHandlePrev->m_minEdges[axis]++;

		pHandleEdge->m_minEdges[axis]--;

		
		Edge swap = *pEdge;
		*pEdge = *pPrev;
		*pPrev = swap;

		
		pEdge--;
		pPrev--;
	}

#ifdef DEBUG_BROADPHASE
	debugPrintAxis(axis);
#endif  
}


template <typename BP_FP_INT_TYPE>
void btAxisSweep3Internal<BP_FP_INT_TYPE>::sortMinUp(int axis, BP_FP_INT_TYPE edge, btDispatcher* dispatcher, bool updateOverlaps)
{
	Edge* pEdge = m_pEdges[axis] + edge;
	Edge* pNext = pEdge + 1;
	Handle* pHandleEdge = getHandle(pEdge->m_handle);

	while (pNext->m_handle && (pEdge->m_pos >= pNext->m_pos))
	{
		Handle* pHandleNext = getHandle(pNext->m_handle);

		if (pNext->IsMax())
		{
			Handle* handle0 = getHandle(pEdge->m_handle);
			Handle* handle1 = getHandle(pNext->m_handle);
			const int axis1 = (1 << axis) & 3;
			const int axis2 = (1 << axis1) & 3;

			
			if (updateOverlaps
#ifdef USE_OVERLAP_TEST_ON_REMOVES
				&& testOverlap2D(handle0, handle1, axis1, axis2)
#endif  
			)
			{
				m_pairCache->removeOverlappingPair(handle0, handle1, dispatcher);
				if (m_userPairCallback)
					m_userPairCallback->removeOverlappingPair(handle0, handle1, dispatcher);
			}

			
			pHandleNext->m_maxEdges[axis]--;
		}
		else
			pHandleNext->m_minEdges[axis]--;

		pHandleEdge->m_minEdges[axis]++;

		
		Edge swap = *pEdge;
		*pEdge = *pNext;
		*pNext = swap;

		
		pEdge++;
		pNext++;
	}
}


template <typename BP_FP_INT_TYPE>
void btAxisSweep3Internal<BP_FP_INT_TYPE>::sortMaxDown(int axis, BP_FP_INT_TYPE edge, btDispatcher* dispatcher, bool updateOverlaps)
{
	Edge* pEdge = m_pEdges[axis] + edge;
	Edge* pPrev = pEdge - 1;
	Handle* pHandleEdge = getHandle(pEdge->m_handle);

	while (pEdge->m_pos < pPrev->m_pos)
	{
		Handle* pHandlePrev = getHandle(pPrev->m_handle);

		if (!pPrev->IsMax())
		{
			
			Handle* handle0 = getHandle(pEdge->m_handle);
			Handle* handle1 = getHandle(pPrev->m_handle);
			const int axis1 = (1 << axis) & 3;
			const int axis2 = (1 << axis1) & 3;

			if (updateOverlaps
#ifdef USE_OVERLAP_TEST_ON_REMOVES
				&& testOverlap2D(handle0, handle1, axis1, axis2)
#endif  
			)
			{
				

				m_pairCache->removeOverlappingPair(handle0, handle1, dispatcher);
				if (m_userPairCallback)
					m_userPairCallback->removeOverlappingPair(handle0, handle1, dispatcher);
			}

			
			pHandlePrev->m_minEdges[axis]++;
			;
		}
		else
			pHandlePrev->m_maxEdges[axis]++;

		pHandleEdge->m_maxEdges[axis]--;

		
		Edge swap = *pEdge;
		*pEdge = *pPrev;
		*pPrev = swap;

		
		pEdge--;
		pPrev--;
	}

#ifdef DEBUG_BROADPHASE
	debugPrintAxis(axis);
#endif  
}


template <typename BP_FP_INT_TYPE>
void btAxisSweep3Internal<BP_FP_INT_TYPE>::sortMaxUp(int axis, BP_FP_INT_TYPE edge, btDispatcher* , bool updateOverlaps)
{
	Edge* pEdge = m_pEdges[axis] + edge;
	Edge* pNext = pEdge + 1;
	Handle* pHandleEdge = getHandle(pEdge->m_handle);

	while (pNext->m_handle && (pEdge->m_pos >= pNext->m_pos))
	{
		Handle* pHandleNext = getHandle(pNext->m_handle);

		const int axis1 = (1 << axis) & 3;
		const int axis2 = (1 << axis1) & 3;

		if (!pNext->IsMax())
		{
			
			if (updateOverlaps && testOverlap2D(pHandleEdge, pHandleNext, axis1, axis2))
			{
				Handle* handle0 = getHandle(pEdge->m_handle);
				Handle* handle1 = getHandle(pNext->m_handle);
				m_pairCache->addOverlappingPair(handle0, handle1);
				if (m_userPairCallback)
					m_userPairCallback->addOverlappingPair(handle0, handle1);
			}

			
			pHandleNext->m_minEdges[axis]--;
		}
		else
			pHandleNext->m_maxEdges[axis]--;

		pHandleEdge->m_maxEdges[axis]++;

		
		Edge swap = *pEdge;
		*pEdge = *pNext;
		*pNext = swap;

		
		pEdge++;
		pNext++;
	}
}

#endif
