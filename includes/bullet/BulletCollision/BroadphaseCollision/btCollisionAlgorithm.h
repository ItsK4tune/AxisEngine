

#ifndef BT_COLLISION_ALGORITHM_H
#define BT_COLLISION_ALGORITHM_H

#include "LinearMath/btScalar.h"
#include "LinearMath/btAlignedObjectArray.h"

struct btBroadphaseProxy;
class btDispatcher;
class btManifoldResult;
class btCollisionObject;
struct btCollisionObjectWrapper;
struct btDispatcherInfo;
class btPersistentManifold;

typedef btAlignedObjectArray<btPersistentManifold*> btManifoldArray;

struct btCollisionAlgorithmConstructionInfo
{
	btCollisionAlgorithmConstructionInfo()
		: m_dispatcher1(0),
		  m_manifold(0)
	{
	}
	btCollisionAlgorithmConstructionInfo(btDispatcher* dispatcher, int temp)
		: m_dispatcher1(dispatcher)
	{
		(void)temp;
	}

	btDispatcher* m_dispatcher1;
	btPersistentManifold* m_manifold;

	
};



class btCollisionAlgorithm
{
protected:
	btDispatcher* m_dispatcher;

protected:
	

public:
	btCollisionAlgorithm(){};

	btCollisionAlgorithm(const btCollisionAlgorithmConstructionInfo& ci);

	virtual ~btCollisionAlgorithm(){};

	virtual void processCollision(const btCollisionObjectWrapper* body0Wrap, const btCollisionObjectWrapper* body1Wrap, const btDispatcherInfo& dispatchInfo, btManifoldResult* resultOut) = 0;

	virtual btScalar calculateTimeOfImpact(btCollisionObject* body0, btCollisionObject* body1, const btDispatcherInfo& dispatchInfo, btManifoldResult* resultOut) = 0;

	virtual void getAllContactManifolds(btManifoldArray& manifoldArray) = 0;
};

#endif  
