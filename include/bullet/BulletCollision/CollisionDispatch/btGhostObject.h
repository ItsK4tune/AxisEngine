

#ifndef BT_GHOST_OBJECT_H
#define BT_GHOST_OBJECT_H

#include "btCollisionObject.h"
#include "BulletCollision/BroadphaseCollision/btOverlappingPairCallback.h"
#include "LinearMath/btAlignedAllocator.h"
#include "BulletCollision/BroadphaseCollision/btOverlappingPairCache.h"
#include "btCollisionWorld.h"

class btConvexShape;

class btDispatcher;





ATTRIBUTE_ALIGNED16(class)
btGhostObject : public btCollisionObject
{
protected:
	btAlignedObjectArray<btCollisionObject*> m_overlappingObjects;

public:
	btGhostObject();

	virtual ~btGhostObject();

	void convexSweepTest(const class btConvexShape* castShape, const btTransform& convexFromWorld, const btTransform& convexToWorld, btCollisionWorld::ConvexResultCallback& resultCallback, btScalar allowedCcdPenetration = 0.f) const;

	void rayTest(const btVector3& rayFromWorld, const btVector3& rayToWorld, btCollisionWorld::RayResultCallback& resultCallback) const;

	
	virtual void addOverlappingObjectInternal(btBroadphaseProxy * otherProxy, btBroadphaseProxy* thisProxy = 0);
	
	virtual void removeOverlappingObjectInternal(btBroadphaseProxy * otherProxy, btDispatcher * dispatcher, btBroadphaseProxy* thisProxy = 0);

	int getNumOverlappingObjects() const
	{
		return m_overlappingObjects.size();
	}

	btCollisionObject* getOverlappingObject(int index)
	{
		return m_overlappingObjects[index];
	}

	const btCollisionObject* getOverlappingObject(int index) const
	{
		return m_overlappingObjects[index];
	}

	btAlignedObjectArray<btCollisionObject*>& getOverlappingPairs()
	{
		return m_overlappingObjects;
	}

	const btAlignedObjectArray<btCollisionObject*> getOverlappingPairs() const
	{
		return m_overlappingObjects;
	}

	
	
	

	static const btGhostObject* upcast(const btCollisionObject* colObj)
	{
		if (colObj->getInternalType() == CO_GHOST_OBJECT)
			return (const btGhostObject*)colObj;
		return 0;
	}
	static btGhostObject* upcast(btCollisionObject * colObj)
	{
		if (colObj->getInternalType() == CO_GHOST_OBJECT)
			return (btGhostObject*)colObj;
		return 0;
	}
};

class btPairCachingGhostObject : public btGhostObject
{
	btHashedOverlappingPairCache* m_hashPairCache;

public:
	btPairCachingGhostObject();

	virtual ~btPairCachingGhostObject();

	
	virtual void addOverlappingObjectInternal(btBroadphaseProxy* otherProxy, btBroadphaseProxy* thisProxy = 0);

	virtual void removeOverlappingObjectInternal(btBroadphaseProxy* otherProxy, btDispatcher* dispatcher, btBroadphaseProxy* thisProxy = 0);

	btHashedOverlappingPairCache* getOverlappingPairCache()
	{
		return m_hashPairCache;
	}
};


class btGhostPairCallback : public btOverlappingPairCallback
{
public:
	btGhostPairCallback()
	{
	}

	virtual ~btGhostPairCallback()
	{
	}

	virtual btBroadphasePair* addOverlappingPair(btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1)
	{
		btCollisionObject* colObj0 = (btCollisionObject*)proxy0->m_clientObject;
		btCollisionObject* colObj1 = (btCollisionObject*)proxy1->m_clientObject;
		btGhostObject* ghost0 = btGhostObject::upcast(colObj0);
		btGhostObject* ghost1 = btGhostObject::upcast(colObj1);
		if (ghost0)
			ghost0->addOverlappingObjectInternal(proxy1, proxy0);
		if (ghost1)
			ghost1->addOverlappingObjectInternal(proxy0, proxy1);
		return 0;
	}

	virtual void* removeOverlappingPair(btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1, btDispatcher* dispatcher)
	{
		btCollisionObject* colObj0 = (btCollisionObject*)proxy0->m_clientObject;
		btCollisionObject* colObj1 = (btCollisionObject*)proxy1->m_clientObject;
		btGhostObject* ghost0 = btGhostObject::upcast(colObj0);
		btGhostObject* ghost1 = btGhostObject::upcast(colObj1);
		if (ghost0)
			ghost0->removeOverlappingObjectInternal(proxy1, dispatcher, proxy0);
		if (ghost1)
			ghost1->removeOverlappingObjectInternal(proxy0, dispatcher, proxy1);
		return 0;
	}

	virtual void removeOverlappingPairsContainingProxy(btBroadphaseProxy* , btDispatcher* )
	{
		btAssert(0);
		
		
	}
};

#endif
