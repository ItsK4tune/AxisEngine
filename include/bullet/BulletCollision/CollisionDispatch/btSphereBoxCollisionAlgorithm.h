

#ifndef BT_SPHERE_BOX_COLLISION_ALGORITHM_H
#define BT_SPHERE_BOX_COLLISION_ALGORITHM_H

#include "btActivatingCollisionAlgorithm.h"
#include "BulletCollision/BroadphaseCollision/btBroadphaseProxy.h"
#include "BulletCollision/CollisionDispatch/btCollisionCreateFunc.h"
class btPersistentManifold;
#include "btCollisionDispatcher.h"

#include "LinearMath/btVector3.h"



class btSphereBoxCollisionAlgorithm : public btActivatingCollisionAlgorithm
{
	bool m_ownManifold;
	btPersistentManifold* m_manifoldPtr;
	bool m_isSwapped;

public:
	btSphereBoxCollisionAlgorithm(btPersistentManifold* mf, const btCollisionAlgorithmConstructionInfo& ci, const btCollisionObjectWrapper* body0Wrap, const btCollisionObjectWrapper* body1Wrap, bool isSwapped);

	virtual ~btSphereBoxCollisionAlgorithm();

	virtual void processCollision(const btCollisionObjectWrapper* body0Wrap, const btCollisionObjectWrapper* body1Wrap, const btDispatcherInfo& dispatchInfo, btManifoldResult* resultOut);

	virtual btScalar calculateTimeOfImpact(btCollisionObject* body0, btCollisionObject* body1, const btDispatcherInfo& dispatchInfo, btManifoldResult* resultOut);

	virtual void getAllContactManifolds(btManifoldArray& manifoldArray)
	{
		if (m_manifoldPtr && m_ownManifold)
		{
			manifoldArray.push_back(m_manifoldPtr);
		}
	}

	bool getSphereDistance(const btCollisionObjectWrapper* boxObjWrap, btVector3& v3PointOnBox, btVector3& normal, btScalar& penetrationDepth, const btVector3& v3SphereCenter, btScalar fRadius, btScalar maxContactDistance);

	btScalar getSpherePenetration(btVector3 const& boxHalfExtent, btVector3 const& sphereRelPos, btVector3& closestPoint, btVector3& normal);

	struct CreateFunc : public btCollisionAlgorithmCreateFunc
	{
		virtual btCollisionAlgorithm* CreateCollisionAlgorithm(btCollisionAlgorithmConstructionInfo& ci, const btCollisionObjectWrapper* body0Wrap, const btCollisionObjectWrapper* body1Wrap)
		{
			void* mem = ci.m_dispatcher1->allocateCollisionAlgorithm(sizeof(btSphereBoxCollisionAlgorithm));
			if (!m_swapped)
			{
				return new (mem) btSphereBoxCollisionAlgorithm(0, ci, body0Wrap, body1Wrap, false);
			}
			else
			{
				return new (mem) btSphereBoxCollisionAlgorithm(0, ci, body0Wrap, body1Wrap, true);
			}
		}
	};
};

#endif  
