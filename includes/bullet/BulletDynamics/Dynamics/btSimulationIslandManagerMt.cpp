

#include "LinearMath/btScalar.h"
#include "LinearMath/btThreads.h"
#include "btSimulationIslandManagerMt.h"
#include "BulletCollision/BroadphaseCollision/btDispatcher.h"
#include "BulletCollision/NarrowPhaseCollision/btPersistentManifold.h"
#include "BulletCollision/CollisionDispatch/btCollisionObject.h"
#include "BulletCollision/CollisionDispatch/btCollisionWorld.h"
#include "BulletDynamics/ConstraintSolver/btTypedConstraint.h"
#include "BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolverMt.h"  


#include "LinearMath/btQuickprof.h"

SIMD_FORCE_INLINE int calcBatchCost(int bodies, int manifolds, int constraints)
{
	
	int batchCost = bodies + 8 * manifolds + 4 * constraints;
	return batchCost;
}

SIMD_FORCE_INLINE int calcBatchCost(const btSimulationIslandManagerMt::Island* island)
{
	return calcBatchCost(island->bodyArray.size(), island->manifoldArray.size(), island->constraintArray.size());
}

btSimulationIslandManagerMt::btSimulationIslandManagerMt()
{
	m_minimumSolverBatchSize = calcBatchCost(0, 128, 0);
	m_batchIslandMinBodyCount = 32;
	m_islandDispatch = parallelIslandDispatch;
	m_batchIsland = NULL;
}

btSimulationIslandManagerMt::~btSimulationIslandManagerMt()
{
	for (int i = 0; i < m_allocatedIslands.size(); ++i)
	{
		delete m_allocatedIslands[i];
	}
	m_allocatedIslands.resize(0);
	m_activeIslands.resize(0);
	m_freeIslands.resize(0);
}

inline int getIslandId(const btPersistentManifold* lhs)
{
	const btCollisionObject* rcolObj0 = static_cast<const btCollisionObject*>(lhs->getBody0());
	const btCollisionObject* rcolObj1 = static_cast<const btCollisionObject*>(lhs->getBody1());
	int islandId = rcolObj0->getIslandTag() >= 0 ? rcolObj0->getIslandTag() : rcolObj1->getIslandTag();
	return islandId;
}

SIMD_FORCE_INLINE int btGetConstraintIslandId1(const btTypedConstraint* lhs)
{
	const btCollisionObject& rcolObj0 = lhs->getRigidBodyA();
	const btCollisionObject& rcolObj1 = lhs->getRigidBodyB();
	int islandId = rcolObj0.getIslandTag() >= 0 ? rcolObj0.getIslandTag() : rcolObj1.getIslandTag();
	return islandId;
}


class IslandBatchSizeSortPredicate
{
public:
	bool operator()(const btSimulationIslandManagerMt::Island* lhs, const btSimulationIslandManagerMt::Island* rhs) const
	{
		int lCost = calcBatchCost(lhs);
		int rCost = calcBatchCost(rhs);
		return lCost > rCost;
	}
};

class IslandBodyCapacitySortPredicate
{
public:
	bool operator()(const btSimulationIslandManagerMt::Island* lhs, const btSimulationIslandManagerMt::Island* rhs) const
	{
		return lhs->bodyArray.capacity() > rhs->bodyArray.capacity();
	}
};

void btSimulationIslandManagerMt::Island::append(const Island& other)
{
	
	for (int i = 0; i < other.bodyArray.size(); ++i)
	{
		bodyArray.push_back(other.bodyArray[i]);
	}
	
	for (int i = 0; i < other.manifoldArray.size(); ++i)
	{
		manifoldArray.push_back(other.manifoldArray[i]);
	}
	
	for (int i = 0; i < other.constraintArray.size(); ++i)
	{
		constraintArray.push_back(other.constraintArray[i]);
	}
}

bool btIsBodyInIsland(const btSimulationIslandManagerMt::Island& island, const btCollisionObject* obj)
{
	for (int i = 0; i < island.bodyArray.size(); ++i)
	{
		if (island.bodyArray[i] == obj)
		{
			return true;
		}
	}
	return false;
}

void btSimulationIslandManagerMt::initIslandPools()
{
	
	int numElem = getUnionFind().getNumElements();
	m_lookupIslandFromId.resize(numElem);
	for (int i = 0; i < m_lookupIslandFromId.size(); ++i)
	{
		m_lookupIslandFromId[i] = NULL;
	}
	m_activeIslands.resize(0);
	m_freeIslands.resize(0);
	
	int lastCapacity = 0;
	bool isSorted = true;
	for (int i = 0; i < m_allocatedIslands.size(); ++i)
	{
		Island* island = m_allocatedIslands[i];
		int cap = island->bodyArray.capacity();
		if (cap > lastCapacity)
		{
			isSorted = false;
			break;
		}
		lastCapacity = cap;
	}
	if (!isSorted)
	{
		m_allocatedIslands.quickSort(IslandBodyCapacitySortPredicate());
	}

	m_batchIsland = NULL;
	
	for (int i = 0; i < m_allocatedIslands.size(); ++i)
	{
		Island* island = m_allocatedIslands[i];
		island->bodyArray.resize(0);
		island->manifoldArray.resize(0);
		island->constraintArray.resize(0);
		island->id = -1;
		island->isSleeping = true;
		m_freeIslands.push_back(island);
	}
}

btSimulationIslandManagerMt::Island* btSimulationIslandManagerMt::getIsland(int id)
{
	btAssert(id >= 0);
	btAssert(id < m_lookupIslandFromId.size());
	Island* island = m_lookupIslandFromId[id];
	if (island == NULL)
	{
		
		for (int i = 0; i < m_activeIslands.size(); ++i)
		{
			if (m_activeIslands[i]->id == id)
			{
				island = m_activeIslands[i];
				break;
			}
		}
		m_lookupIslandFromId[id] = island;
	}
	return island;
}

btSimulationIslandManagerMt::Island* btSimulationIslandManagerMt::allocateIsland(int id, int numBodies)
{
	Island* island = NULL;
	int allocSize = numBodies;
	if (numBodies < m_batchIslandMinBodyCount)
	{
		if (m_batchIsland)
		{
			island = m_batchIsland;
			m_lookupIslandFromId[id] = island;
			
			if (island->bodyArray.size() + numBodies >= m_batchIslandMinBodyCount)
			{
				
				m_batchIsland = NULL;
			}
			return island;
		}
		else
		{
			
			allocSize = m_batchIslandMinBodyCount * 2;
		}
	}
	btAlignedObjectArray<Island*>& freeIslands = m_freeIslands;

	
	if (freeIslands.size() > 0)
	{
		
		int iFound = freeIslands.size();
		
		for (int i = freeIslands.size() - 1; i >= 0; --i)
		{
			if (freeIslands[i]->bodyArray.capacity() >= allocSize)
			{
				iFound = i;
				island = freeIslands[i];
				island->id = id;
				break;
			}
		}
		
		if (island)
		{
			int iDest = iFound;
			int iSrc = iDest + 1;
			while (iSrc < freeIslands.size())
			{
				freeIslands[iDest++] = freeIslands[iSrc++];
			}
			freeIslands.pop_back();
		}
	}
	if (island == NULL)
	{
		
		island = new Island();  
		island->id = id;
		island->bodyArray.reserve(allocSize);
		m_allocatedIslands.push_back(island);
	}
	m_lookupIslandFromId[id] = island;
	if (numBodies < m_batchIslandMinBodyCount)
	{
		m_batchIsland = island;
	}
	m_activeIslands.push_back(island);
	return island;
}

void btSimulationIslandManagerMt::buildIslands(btDispatcher* dispatcher, btCollisionWorld* collisionWorld)
{
	BT_PROFILE("buildIslands");

	btCollisionObjectArray& collisionObjects = collisionWorld->getCollisionObjectArray();

	
	

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
}

void btSimulationIslandManagerMt::addBodiesToIslands(btCollisionWorld* collisionWorld)
{
	btCollisionObjectArray& collisionObjects = collisionWorld->getCollisionObjectArray();
	int endIslandIndex = 1;
	int startIslandIndex;
	int numElem = getUnionFind().getNumElements();

	
	for (startIslandIndex = 0; startIslandIndex < numElem; startIslandIndex = endIslandIndex)
	{
		int islandId = getUnionFind().getElement(startIslandIndex).m_id;

		
		for (endIslandIndex = startIslandIndex; (endIslandIndex < numElem) && (getUnionFind().getElement(endIslandIndex).m_id == islandId); endIslandIndex++)
		{
		}
		
		bool islandSleeping = true;
		for (int iElem = startIslandIndex; iElem < endIslandIndex; iElem++)
		{
			int i = getUnionFind().getElement(iElem).m_sz;
			btCollisionObject* colObj = collisionObjects[i];
			if (colObj->isActive())
			{
				islandSleeping = false;
			}
		}
		if (!islandSleeping)
		{
			
			int numBodies = endIslandIndex - startIslandIndex;
			Island* island = allocateIsland(islandId, numBodies);
			island->isSleeping = false;

			
			for (int iElem = startIslandIndex; iElem < endIslandIndex; iElem++)
			{
				int i = getUnionFind().getElement(iElem).m_sz;
				btCollisionObject* colObj = collisionObjects[i];
				island->bodyArray.push_back(colObj);
			}
		}
	}
}

void btSimulationIslandManagerMt::addManifoldsToIslands(btDispatcher* dispatcher)
{
	
	int maxNumManifolds = dispatcher->getNumManifolds();
	for (int i = 0; i < maxNumManifolds; i++)
	{
		btPersistentManifold* manifold = dispatcher->getManifoldByIndexInternal(i);

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
			
			if (dispatcher->needsResponse(colObj0, colObj1))
			{
				
				int islandId = getIslandId(manifold);
				
				if (Island* island = getIsland(islandId))
				{
					island->manifoldArray.push_back(manifold);
				}
			}
		}
	}
}

void btSimulationIslandManagerMt::addConstraintsToIslands(btAlignedObjectArray<btTypedConstraint*>& constraints)
{
	
	for (int i = 0; i < constraints.size(); i++)
	{
		
		btTypedConstraint* constraint = constraints[i];
		if (constraint->isEnabled())
		{
			int islandId = btGetConstraintIslandId1(constraint);
			
			if (Island* island = getIsland(islandId))
			{
				island->constraintArray.push_back(constraint);
			}
		}
	}
}

void btSimulationIslandManagerMt::mergeIslands()
{
	
	m_activeIslands.quickSort(IslandBatchSizeSortPredicate());

	
	
	int destIslandIndex = m_activeIslands.size();
	for (int i = 0; i < m_activeIslands.size(); ++i)
	{
		Island* island = m_activeIslands[i];
		int batchSize = calcBatchCost(island);
		if (batchSize < m_minimumSolverBatchSize)
		{
			destIslandIndex = i;
			break;
		}
	}
	int lastIndex = m_activeIslands.size() - 1;
	while (destIslandIndex < lastIndex)
	{
		
		Island* island = m_activeIslands[destIslandIndex];
		int numBodies = island->bodyArray.size();
		int numManifolds = island->manifoldArray.size();
		int numConstraints = island->constraintArray.size();
		int firstIndex = lastIndex;
		
		while (true)
		{
			Island* src = m_activeIslands[firstIndex];
			numBodies += src->bodyArray.size();
			numManifolds += src->manifoldArray.size();
			numConstraints += src->constraintArray.size();
			int batchCost = calcBatchCost(numBodies, numManifolds, numConstraints);
			if (batchCost >= m_minimumSolverBatchSize)
			{
				break;
			}
			if (firstIndex - 1 == destIslandIndex)
			{
				break;
			}
			firstIndex--;
		}
		
		island->bodyArray.reserve(numBodies);
		island->manifoldArray.reserve(numManifolds);
		island->constraintArray.reserve(numConstraints);
		
		for (int i = firstIndex; i <= lastIndex; ++i)
		{
			island->append(*m_activeIslands[i]);
		}
		
		m_activeIslands.resize(firstIndex);
		lastIndex = firstIndex - 1;
		destIslandIndex++;
	}
}

void btSimulationIslandManagerMt::solveIsland(btConstraintSolver* solver, Island& island, const SolverParams& solverParams)
{
	btPersistentManifold** manifolds = island.manifoldArray.size() ? &island.manifoldArray[0] : NULL;
	btTypedConstraint** constraintsPtr = island.constraintArray.size() ? &island.constraintArray[0] : NULL;
	solver->solveGroup(&island.bodyArray[0],
					   island.bodyArray.size(),
					   manifolds,
					   island.manifoldArray.size(),
					   constraintsPtr,
					   island.constraintArray.size(),
					   *solverParams.m_solverInfo,
					   solverParams.m_debugDrawer,
					   solverParams.m_dispatcher);
}

void btSimulationIslandManagerMt::serialIslandDispatch(btAlignedObjectArray<Island*>* islandsPtr, const SolverParams& solverParams)
{
	BT_PROFILE("serialIslandDispatch");
	
	btAlignedObjectArray<Island*>& islands = *islandsPtr;
	btConstraintSolver* solver = solverParams.m_solverMt ? solverParams.m_solverMt : solverParams.m_solverPool;
	for (int i = 0; i < islands.size(); ++i)
	{
		solveIsland(solver, *islands[i], solverParams);
	}
}

struct UpdateIslandDispatcher : public btIParallelForBody
{
	btAlignedObjectArray<btSimulationIslandManagerMt::Island*>& m_islandsPtr;
	const btSimulationIslandManagerMt::SolverParams& m_solverParams;

	UpdateIslandDispatcher(btAlignedObjectArray<btSimulationIslandManagerMt::Island*>& islandsPtr, const btSimulationIslandManagerMt::SolverParams& solverParams)
		: m_islandsPtr(islandsPtr), m_solverParams(solverParams)
	{
	}

	void forLoop(int iBegin, int iEnd) const BT_OVERRIDE
	{
		btConstraintSolver* solver = m_solverParams.m_solverPool;
		for (int i = iBegin; i < iEnd; ++i)
		{
			btSimulationIslandManagerMt::Island* island = m_islandsPtr[i];
			btSimulationIslandManagerMt::solveIsland(solver, *island, m_solverParams);
		}
	}
};

void btSimulationIslandManagerMt::parallelIslandDispatch(btAlignedObjectArray<Island*>* islandsPtr, const SolverParams& solverParams)
{
	BT_PROFILE("parallelIslandDispatch");
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	

	UpdateIslandDispatcher dispatcher(*islandsPtr, solverParams);
	
	int iBegin = 0;
	if (solverParams.m_solverMt)
	{
		while (iBegin < islandsPtr->size())
		{
			btSimulationIslandManagerMt::Island* island = (*islandsPtr)[iBegin];
			if (island->manifoldArray.size() < btSequentialImpulseConstraintSolverMt::s_minimumContactManifoldsForBatching)
			{
				
				break;
			}
			
			solveIsland(solverParams.m_solverMt, *island, solverParams);
			++iBegin;
		}
	}
	
	btParallelFor(iBegin, islandsPtr->size(), 1, dispatcher);
}


void btSimulationIslandManagerMt::buildAndProcessIslands(btDispatcher* dispatcher,
														 btCollisionWorld* collisionWorld,
														 btAlignedObjectArray<btTypedConstraint*>& constraints,
														 const SolverParams& solverParams)
{
	BT_PROFILE("buildAndProcessIslands");
	btCollisionObjectArray& collisionObjects = collisionWorld->getCollisionObjectArray();

	buildIslands(dispatcher, collisionWorld);

	if (!getSplitIslands())
	{
		btPersistentManifold** manifolds = dispatcher->getInternalManifoldPointer();
		int maxNumManifolds = dispatcher->getNumManifolds();

		for (int i = 0; i < maxNumManifolds; i++)
		{
			btPersistentManifold* manifold = manifolds[i];

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
			}
		}
		btTypedConstraint** constraintsPtr = constraints.size() ? &constraints[0] : NULL;
		btConstraintSolver* solver = solverParams.m_solverMt ? solverParams.m_solverMt : solverParams.m_solverPool;
		solver->solveGroup(&collisionObjects[0],
						   collisionObjects.size(),
						   manifolds,
						   maxNumManifolds,
						   constraintsPtr,
						   constraints.size(),
						   *solverParams.m_solverInfo,
						   solverParams.m_debugDrawer,
						   solverParams.m_dispatcher);
	}
	else
	{
		initIslandPools();

		
		addBodiesToIslands(collisionWorld);
		addManifoldsToIslands(dispatcher);
		addConstraintsToIslands(constraints);

		
		

		
		if (m_minimumSolverBatchSize > 1)
		{
			mergeIslands();
		}
		
		m_islandDispatch(&m_activeIslands, solverParams);
	}
}
