

#include "btCollisionDispatcherMt.h"
#include "LinearMath/btQuickprof.h"

#include "BulletCollision/BroadphaseCollision/btCollisionAlgorithm.h"

#include "BulletCollision/CollisionShapes/btCollisionShape.h"
#include "BulletCollision/CollisionDispatch/btCollisionObject.h"
#include "BulletCollision/BroadphaseCollision/btOverlappingPairCache.h"
#include "LinearMath/btPoolAllocator.h"
#include "BulletCollision/CollisionDispatch/btCollisionConfiguration.h"
#include "BulletCollision/CollisionDispatch/btCollisionObjectWrapper.h"

btCollisionDispatcherMt::btCollisionDispatcherMt(btCollisionConfiguration* config, int grainSize)
	: btCollisionDispatcher(config)
{
	m_batchManifoldsPtr.resize(btGetTaskScheduler()->getNumThreads());
	m_batchReleasePtr.resize(btGetTaskScheduler()->getNumThreads());

	m_batchUpdating = false;
	m_grainSize = grainSize;  
}

btPersistentManifold* btCollisionDispatcherMt::getNewManifold(const btCollisionObject* body0, const btCollisionObject* body1)
{
	

	btScalar contactBreakingThreshold = (m_dispatcherFlags & btCollisionDispatcher::CD_USE_RELATIVE_CONTACT_BREAKING_THRESHOLD) ? btMin(body0->getCollisionShape()->getContactBreakingThreshold(gContactBreakingThreshold), body1->getCollisionShape()->getContactBreakingThreshold(gContactBreakingThreshold))
																																: gContactBreakingThreshold;

	btScalar contactProcessingThreshold = btMin(body0->getContactProcessingThreshold(), body1->getContactProcessingThreshold());

	void* mem = m_persistentManifoldPoolAllocator->allocate(sizeof(btPersistentManifold));
	if (NULL == mem)
	{
		
		if ((m_dispatcherFlags & CD_DISABLE_CONTACTPOOL_DYNAMIC_ALLOCATION) == 0)
		{
			mem = btAlignedAlloc(sizeof(btPersistentManifold), 16);
		}
		else
		{
			btAssert(0);
			
			return 0;
		}
	}
	btPersistentManifold* manifold = new (mem) btPersistentManifold(body0, body1, 0, contactBreakingThreshold, contactProcessingThreshold);
	if (!m_batchUpdating)
	{
		
		
		
		manifold->m_index1a = m_manifoldsPtr.size();
		m_manifoldsPtr.push_back(manifold);
	}
	else
	{
		m_batchManifoldsPtr[btGetCurrentThreadIndex()].push_back(manifold);
	}

	return manifold;
}

void btCollisionDispatcherMt::releaseManifold(btPersistentManifold* manifold)
{
	
	
	if (!m_batchUpdating)
	{
		clearManifold(manifold);
		
		
		int findIndex = manifold->m_index1a;
		btAssert(findIndex < m_manifoldsPtr.size());
		m_manifoldsPtr.swap(findIndex, m_manifoldsPtr.size() - 1);
		m_manifoldsPtr[findIndex]->m_index1a = findIndex;
		m_manifoldsPtr.pop_back();
	} else {
		m_batchReleasePtr[btGetCurrentThreadIndex()].push_back(manifold);
		return;
	}

	manifold->~btPersistentManifold();
	if (m_persistentManifoldPoolAllocator->validPtr(manifold))
	{
		m_persistentManifoldPoolAllocator->freeMemory(manifold);
	}
	else
	{
		btAlignedFree(manifold);
	}
}

struct CollisionDispatcherUpdater : public btIParallelForBody
{
	btBroadphasePair* mPairArray;
	btNearCallback mCallback;
	btCollisionDispatcher* mDispatcher;
	const btDispatcherInfo* mInfo;

	CollisionDispatcherUpdater()
	{
		mPairArray = NULL;
		mCallback = NULL;
		mDispatcher = NULL;
		mInfo = NULL;
	}
	void forLoop(int iBegin, int iEnd) const
	{
		for (int i = iBegin; i < iEnd; ++i)
		{
			btBroadphasePair* pair = &mPairArray[i];
			mCallback(*pair, *mDispatcher, *mInfo);
		}
	}
};

void btCollisionDispatcherMt::dispatchAllCollisionPairs(btOverlappingPairCache* pairCache, const btDispatcherInfo& info, btDispatcher* dispatcher)
{
	const int pairCount = pairCache->getNumOverlappingPairs();
	if (pairCount == 0)
	{
		return;
	}
	CollisionDispatcherUpdater updater;
	updater.mCallback = getNearCallback();
	updater.mPairArray = pairCache->getOverlappingPairArrayPtr();
	updater.mDispatcher = this;
	updater.mInfo = &info;

	m_batchUpdating = true;
	btParallelFor(0, pairCount, m_grainSize, updater);
	m_batchUpdating = false;

	
	for (int i = 0; i < m_batchManifoldsPtr.size(); ++i)
	{
		btAlignedObjectArray<btPersistentManifold*>& batchManifoldsPtr = m_batchManifoldsPtr[i];

		for (int j = 0; j < batchManifoldsPtr.size(); ++j)
		{
			m_manifoldsPtr.push_back(batchManifoldsPtr[j]);
		}

		batchManifoldsPtr.resizeNoInitialize(0);
	}

	
	for (int i = 0; i < m_batchReleasePtr.size(); ++i)
	{
		btAlignedObjectArray<btPersistentManifold*>& batchManifoldsPtr = m_batchReleasePtr[i];
		for (int j = 0; j < batchManifoldsPtr.size(); ++j)
		{
			releaseManifold(batchManifoldsPtr[j]);
		}
		batchManifoldsPtr.resizeNoInitialize(0);
	}

	
	for (int i = 0; i < m_manifoldsPtr.size(); ++i)
	{
		m_manifoldsPtr[i]->m_index1a = i;
	}
}
