

#ifndef BT_COLLISION_DISPATCHER_MT_H
#define BT_COLLISION_DISPATCHER_MT_H

#include "BulletCollision/CollisionDispatch/btCollisionDispatcher.h"
#include "LinearMath/btThreads.h"

class btCollisionDispatcherMt : public btCollisionDispatcher
{
public:
	btCollisionDispatcherMt(btCollisionConfiguration* config, int grainSize = 40);

	virtual btPersistentManifold* getNewManifold(const btCollisionObject* body0, const btCollisionObject* body1) BT_OVERRIDE;
	virtual void releaseManifold(btPersistentManifold* manifold) BT_OVERRIDE;

	virtual void dispatchAllCollisionPairs(btOverlappingPairCache* pairCache, const btDispatcherInfo& info, btDispatcher* dispatcher) BT_OVERRIDE;

protected:
	btAlignedObjectArray<btAlignedObjectArray<btPersistentManifold*> > m_batchManifoldsPtr;
	btAlignedObjectArray<btAlignedObjectArray<btPersistentManifold*> > m_batchReleasePtr;
	bool m_batchUpdating;
	int m_grainSize;
};

#endif  
