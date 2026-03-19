

#ifndef BT_SIMULATION_ISLAND_MANAGER_H
#define BT_SIMULATION_ISLAND_MANAGER_H

#include "BulletCollision/CollisionDispatch/btUnionFind.h"
#include "btCollisionCreateFunc.h"
#include "LinearMath/btAlignedObjectArray.h"
#include "btCollisionObject.h"

class btCollisionObject;
class btCollisionWorld;
class btDispatcher;
class btPersistentManifold;


class btSimulationIslandManager
{
	btUnionFind m_unionFind;

	btAlignedObjectArray<btPersistentManifold*> m_islandmanifold;
	btAlignedObjectArray<btCollisionObject*> m_islandBodies;

	bool m_splitIslands;

public:
	btSimulationIslandManager();
	virtual ~btSimulationIslandManager();

	void initUnionFind(int n);

	btUnionFind& getUnionFind() { return m_unionFind; }

	virtual void updateActivationState(btCollisionWorld* colWorld, btDispatcher* dispatcher);
	virtual void storeIslandActivationState(btCollisionWorld* world);

	void findUnions(btDispatcher* dispatcher, btCollisionWorld* colWorld);

	struct IslandCallback
	{
		virtual ~IslandCallback(){};

		virtual void processIsland(btCollisionObject** bodies, int numBodies, class btPersistentManifold** manifolds, int numManifolds, int islandId) = 0;
	};

	void buildAndProcessIslands(btDispatcher* dispatcher, btCollisionWorld* collisionWorld, IslandCallback* callback);
    
	void buildIslands(btDispatcher* dispatcher, btCollisionWorld* colWorld);

    void processIslands(btDispatcher* dispatcher, btCollisionWorld* collisionWorld, IslandCallback* callback);
    
	bool getSplitIslands()
	{
		return m_splitIslands;
	}
	void setSplitIslands(bool doSplitIslands)
	{
		m_splitIslands = doSplitIslands;
	}
};

#endif  
