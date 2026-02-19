

#ifndef BT_SIMULATION_ISLAND_MANAGER_MT_H
#define BT_SIMULATION_ISLAND_MANAGER_MT_H

#include "BulletCollision/CollisionDispatch/btSimulationIslandManager.h"

class btTypedConstraint;
class btConstraintSolver;
struct btContactSolverInfo;
class btIDebugDraw;










class btSimulationIslandManagerMt : public btSimulationIslandManager
{
public:
	struct Island
	{
		
		
		btAlignedObjectArray<btCollisionObject*> bodyArray;
		btAlignedObjectArray<btPersistentManifold*> manifoldArray;
		btAlignedObjectArray<btTypedConstraint*> constraintArray;
		int id;  
		bool isSleeping;

		void append(const Island& other);  
	};
	struct SolverParams
	{
		btConstraintSolver* m_solverPool;
		btConstraintSolver* m_solverMt;
		btContactSolverInfo* m_solverInfo;
		btIDebugDraw* m_debugDrawer;
		btDispatcher* m_dispatcher;
	};
	static void solveIsland(btConstraintSolver* solver, Island& island, const SolverParams& solverParams);

	typedef void (*IslandDispatchFunc)(btAlignedObjectArray<Island*>* islands, const SolverParams& solverParams);
	static void serialIslandDispatch(btAlignedObjectArray<Island*>* islandsPtr, const SolverParams& solverParams);
	static void parallelIslandDispatch(btAlignedObjectArray<Island*>* islandsPtr, const SolverParams& solverParams);

protected:
	btAlignedObjectArray<Island*> m_allocatedIslands;    
	btAlignedObjectArray<Island*> m_activeIslands;       
	btAlignedObjectArray<Island*> m_freeIslands;         
	btAlignedObjectArray<Island*> m_lookupIslandFromId;  
	Island* m_batchIsland;
	int m_minimumSolverBatchSize;
	int m_batchIslandMinBodyCount;
	IslandDispatchFunc m_islandDispatch;

	Island* getIsland(int id);
	virtual Island* allocateIsland(int id, int numBodies);
	virtual void initIslandPools();
	virtual void addBodiesToIslands(btCollisionWorld* collisionWorld);
	virtual void addManifoldsToIslands(btDispatcher* dispatcher);
	virtual void addConstraintsToIslands(btAlignedObjectArray<btTypedConstraint*>& constraints);
	virtual void mergeIslands();

public:
	btSimulationIslandManagerMt();
	virtual ~btSimulationIslandManagerMt();

	virtual void buildAndProcessIslands(btDispatcher* dispatcher,
										btCollisionWorld* collisionWorld,
										btAlignedObjectArray<btTypedConstraint*>& constraints,
										const SolverParams& solverParams);

	virtual void buildIslands(btDispatcher* dispatcher, btCollisionWorld* colWorld);

	int getMinimumSolverBatchSize() const
	{
		return m_minimumSolverBatchSize;
	}
	void setMinimumSolverBatchSize(int sz)
	{
		m_minimumSolverBatchSize = sz;
	}
	IslandDispatchFunc getIslandDispatchFunction() const
	{
		return m_islandDispatch;
	}
	
	void setIslandDispatchFunction(IslandDispatchFunc func)
	{
		m_islandDispatch = func;
	}
};

#endif  
