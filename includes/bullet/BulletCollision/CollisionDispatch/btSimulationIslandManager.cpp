


#include "LinearMath/btScalar.h"
#include "btSimulationIslandManager.h"
#include "BulletCollision/BroadphaseCollision/btDispatcher.h"
#include "BulletCollision/NarrowPhaseCollision/btPersistentManifold.h"
#include "BulletCollision/CollisionDispatch/btCollisionObject.h"
#include "BulletCollision/CollisionDispatch/btCollisionWorld.h"


#include "LinearMath/btQuickprof.h"

btSimulationIslandManager::btSimulationIslandManager() : m_splitIslands(true)
{
}

btSimulationIslandManager::~btSimulationIslandManager()
{
}

void btSimulationIslandManager::initUnionFind(int n)
{
	m_unionFind.reset(n);
}

void btSimulationIslandManager::findUnions(btDispatcher* , btCollisionWorld* colWorld)
{
	{
		btOverlappingPairCache* pairCachePtr = colWorld->getPairCache();
		const int numOverlappingPairs = pairCachePtr->getNumOverlappingPairs();
		if (numOverlappingPairs)
		{
			btBroadphasePair* pairPtr = pairCachePtr->getOverlappingPairArrayPtr();

			for (int i = 0; i < numOverlappingPairs; i++)
			{
				const btBroadphasePair& collisionPair = pairPtr[i];
				btCollisionObject* colObj0 = (btCollisionObject*)collisionPair.m_pProxy0->m_clientObject;
				btCollisionObject* colObj1 = (btCollisionObject*)collisionPair.m_pProxy1->m_clientObject;

				if (((colObj0) && ((colObj0)->mergesSimulationIslands())) &&
					((colObj1) && ((colObj1)->mergesSimulationIslands())))
				{
					m_unionFind.unite((colObj0)->getIslandTag(),
									  (colObj1)->getIslandTag());
				}
			}
		}
	}
}

#ifdef STATIC_SIMULATION_ISLAND_OPTIMIZATION
void btSimulationIslandManager::updateActivationState(btCollisionWorld* colWorld, btDispatcher* dispatcher)
{
	
	int index = 0;
	{
		int i;
		for (i = 0; i < colWorld->getCollisionObjectArray().size(); i++)
		{
			btCollisionObject* collisionObject = colWorld->getCollisionObjectArray()[i];
			
			if (!collisionObject->isStaticOrKinematicObject())
			{
				collisionObject->setIslandTag(index++);
			}
			collisionObject->setCompanionId(-1);
			collisionObject->setHitFraction(btScalar(1.));
		}
	}
	

	initUnionFind(index);

	findUnions(dispatcher, colWorld);
}

void btSimulationIslandManager::storeIslandActivationState(btCollisionWorld* colWorld)
{
	
	{
		int index = 0;
		int i;
		for (i = 0; i < colWorld->getCollisionObjectArray().size(); i++)
		{
			btCollisionObject* collisionObject = colWorld->getCollisionObjectArray()[i];
			if (!collisionObject->isStaticOrKinematicObject())
			{
				collisionObject->setIslandTag(m_unionFind.find(index));
				
				m_unionFind.getElement(index).m_sz = i;
				collisionObject->setCompanionId(-1);
				index++;
			}
			else
			{
				collisionObject->setIslandTag(-1);
				collisionObject->setCompanionId(-2);
			}
		}
	}
}

#else  
void btSimulationIslandManager::updateActivationState(btCollisionWorld* colWorld, btDispatcher* dispatcher)
{
	initUnionFind(int(colWorld->getCollisionObjectArray().size()));

	
	{
		int index = 0;
		int i;
		for (i = 0; i < colWorld->getCollisionObjectArray().size(); i++)
		{
			btCollisionObject* collisionObject = colWorld->getCollisionObjectArray()[i];
			collisionObject->setIslandTag(index);
			collisionObject->setCompanionId(-1);
			collisionObject->setHitFraction(btScalar(1.));
			index++;
		}
	}
	

	findUnions(dispatcher, colWorld);
}

void btSimulationIslandManager::storeIslandActivationState(btCollisionWorld* colWorld)
{
	
	{
		int index = 0;
		int i;
		for (i = 0; i < colWorld->getCollisionObjectArray().size(); i++)
		{
			btCollisionObject* collisionObject = colWorld->getCollisionObjectArray()[i];
			if (!collisionObject->isStaticOrKinematicObject())
			{
				collisionObject->setIslandTag(m_unionFind.find(index));
				collisionObject->setCompanionId(-1);
			}
			else
			{
				collisionObject->setIslandTag(-1);
				collisionObject->setCompanionId(-2);
			}
			index++;
		}
	}
}

#endif  

inline int getIslandId(const btPersistentManifold* lhs)
{
	int islandId;
	const btCollisionObject* rcolObj0 = static_cast<const btCollisionObject*>(lhs->getBody0());
	const btCollisionObject* rcolObj1 = static_cast<const btCollisionObject*>(lhs->getBody1());
	islandId = rcolObj0->getIslandTag() >= 0 ? rcolObj0->getIslandTag() : rcolObj1->getIslandTag();
	return islandId;
}


class btPersistentManifoldSortPredicate
{
public:
	SIMD_FORCE_INLINE bool operator()(const btPersistentManifold* lhs, const btPersistentManifold* rhs) const
	{
		return getIslandId(lhs) < getIslandId(rhs);
	}
};

class btPersistentManifoldSortPredicateDeterministic
{
public:
	SIMD_FORCE_INLINE bool operator()(const btPersistentManifold* lhs, const btPersistentManifold* rhs) const
	{
		return (
			(getIslandId(lhs) < getIslandId(rhs)) || ((getIslandId(lhs) == getIslandId(rhs)) && lhs->getBody0()->getBroadphaseHandle()->m_uniqueId < rhs->getBody0()->getBroadphaseHandle()->m_uniqueId) || ((getIslandId(lhs) == getIslandId(rhs)) && (lhs->getBody0()->getBroadphaseHandle()->m_uniqueId == rhs->getBody0()->getBroadphaseHandle()->m_uniqueId) && (lhs->getBody1()->getBroadphaseHandle()->m_uniqueId < rhs->getBody1()->getBroadphaseHandle()->m_uniqueId)));
	}
};

void btSimulationIslandManager::buildIslands(btDispatcher* dispatcher, btCollisionWorld* collisionWorld)
{
	BT_PROFILE("islandUnionFindAndQuickSort");

	btCollisionObjectArray& collisionObjects = collisionWorld->getCollisionObjectArray();

	m_islandmanifold.resize(0);

	
	

	getUnionFind().sortIslands();
	int numElem = getUnionFind().getNumElements();

	int endIslandIndex = 1;
	int startIslandIndex;

	
	for (startIslandIndex = 0; startIslandIndex < numElem; startIslandIndex = endIslandIndex)
	{
		int islandId = getUnionFind().getElement(startIslandIndex).m_id;
		for (endIslandIndex = startIslandIndex + 1; (endIslandIndex < numElem) && (getUnionFind().getElement(endIslandIndex).m_id == islandId); endIslandIndex++)
		{
		}

		

		bool allSleeping = true;

		int idx;
		for (idx = startIslandIndex; idx < endIslandIndex; idx++)
		{
			int i = getUnionFind().getElement(idx).m_sz;

			btCollisionObject* colObj0 = collisionObjects[i];
			if ((colObj0->getIslandTag() != islandId) && (colObj0->getIslandTag() != -1))
			{
				
			}

            btAssert((colObj0->getIslandTag() == islandId) || (colObj0->getIslandTag() == -1));
			if (colObj0->getIslandTag() == islandId)
			{
				if (colObj0->getActivationState() == ACTIVE_TAG ||
					colObj0->getActivationState() == DISABLE_DEACTIVATION)
				{
					allSleeping = false;
					break;
				}
			}
		}

		if (allSleeping)
		{
			int idx;
			for (idx = startIslandIndex; idx < endIslandIndex; idx++)
			{
				int i = getUnionFind().getElement(idx).m_sz;
				btCollisionObject* colObj0 = collisionObjects[i];
				if ((colObj0->getIslandTag() != islandId) && (colObj0->getIslandTag() != -1))
				{
					
				}

                btAssert((colObj0->getIslandTag() == islandId) || (colObj0->getIslandTag() == -1));

				if (colObj0->getIslandTag() == islandId)
				{
					colObj0->setActivationState(ISLAND_SLEEPING);
				}
			}
		}
		else
		{
			int idx;
			for (idx = startIslandIndex; idx < endIslandIndex; idx++)
			{
				int i = getUnionFind().getElement(idx).m_sz;

				btCollisionObject* colObj0 = collisionObjects[i];
				if ((colObj0->getIslandTag() != islandId) && (colObj0->getIslandTag() != -1))
				{
					
				}

                 btAssert((colObj0->getIslandTag() == islandId) || (colObj0->getIslandTag() == -1));


				if (colObj0->getIslandTag() == islandId)
				{
					if (colObj0->getActivationState() == ISLAND_SLEEPING)
					{
						colObj0->setActivationState(WANTS_DEACTIVATION);
						colObj0->setDeactivationTime(0.f);
					}
				}
			}
		}
	}

	int i;
	int maxNumManifolds = dispatcher->getNumManifolds();

	
	

	

	for (i = 0; i < maxNumManifolds; i++)
	{
		btPersistentManifold* manifold = dispatcher->getManifoldByIndexInternal(i);
		if (collisionWorld->getDispatchInfo().m_deterministicOverlappingPairs)
		{
			if (manifold->getNumContacts() == 0)
				continue;
		}

		const btCollisionObject* colObj0 = static_cast<const btCollisionObject*>(manifold->getBody0());
		const btCollisionObject* colObj1 = static_cast<const btCollisionObject*>(manifold->getBody1());

		
		if (((colObj0) && colObj0->getActivationState() != ISLAND_SLEEPING) ||
			((colObj1) && colObj1->getActivationState() != ISLAND_SLEEPING))
		{
			
			if (colObj0->isKinematicObject() && colObj0->getActivationState() != ISLAND_SLEEPING)
			{
				if (colObj0->hasContactResponse())
					colObj1->activate();
			}
			if (colObj1->isKinematicObject() && colObj1->getActivationState() != ISLAND_SLEEPING)
			{
				if (colObj1->hasContactResponse())
					colObj0->activate();
			}
			if (m_splitIslands)
			{
				
				if (dispatcher->needsResponse(colObj0, colObj1))
					m_islandmanifold.push_back(manifold);
			}
		}
	}
}



void btSimulationIslandManager::buildAndProcessIslands(btDispatcher* dispatcher, btCollisionWorld* collisionWorld, IslandCallback* callback)
{
	buildIslands(dispatcher, collisionWorld);
    processIslands(dispatcher, collisionWorld, callback);
}

void btSimulationIslandManager::processIslands(btDispatcher* dispatcher, btCollisionWorld* collisionWorld, IslandCallback* callback)
{
    btCollisionObjectArray& collisionObjects = collisionWorld->getCollisionObjectArray();
	int endIslandIndex = 1;
	int startIslandIndex;
	int numElem = getUnionFind().getNumElements();

	BT_PROFILE("processIslands");

	if (!m_splitIslands)
	{
		btPersistentManifold** manifold = dispatcher->getInternalManifoldPointer();
		int maxNumManifolds = dispatcher->getNumManifolds();
		callback->processIsland(&collisionObjects[0], collisionObjects.size(), manifold, maxNumManifolds, -1);
	}
	else
	{
		
		
		

		int numManifolds = int(m_islandmanifold.size());

		
		
		
		
		if (collisionWorld->getDispatchInfo().m_deterministicOverlappingPairs)
		{
			m_islandmanifold.quickSort(btPersistentManifoldSortPredicateDeterministic());
		}
		else
		{
			m_islandmanifold.quickSort(btPersistentManifoldSortPredicate());
		}

		

		

		int startManifoldIndex = 0;
		int endManifoldIndex = 1;

		

		

		
		for (startIslandIndex = 0; startIslandIndex < numElem; startIslandIndex = endIslandIndex)
		{
			int islandId = getUnionFind().getElement(startIslandIndex).m_id;

			bool islandSleeping = true;

			for (endIslandIndex = startIslandIndex; (endIslandIndex < numElem) && (getUnionFind().getElement(endIslandIndex).m_id == islandId); endIslandIndex++)
			{
				int i = getUnionFind().getElement(endIslandIndex).m_sz;
				btCollisionObject* colObj0 = collisionObjects[i];
				m_islandBodies.push_back(colObj0);
				if (colObj0->isActive())
					islandSleeping = false;
			}

			
			int numIslandManifolds = 0;
			btPersistentManifold** startManifold = 0;

			if (startManifoldIndex < numManifolds)
			{
				int curIslandId = getIslandId(m_islandmanifold[startManifoldIndex]);
				if (curIslandId == islandId)
				{
					startManifold = &m_islandmanifold[startManifoldIndex];

					for (endManifoldIndex = startManifoldIndex + 1; (endManifoldIndex < numManifolds) && (islandId == getIslandId(m_islandmanifold[endManifoldIndex])); endManifoldIndex++)
					{
					}
					
					numIslandManifolds = endManifoldIndex - startManifoldIndex;
				}
			}

			if (!islandSleeping)
			{
				callback->processIsland(&m_islandBodies[0], m_islandBodies.size(), startManifold, numIslandManifolds, islandId);
				
			}

			if (numIslandManifolds)
			{
				startManifoldIndex = endManifoldIndex;
			}

			m_islandBodies.resize(0);
		}
	}  
}
