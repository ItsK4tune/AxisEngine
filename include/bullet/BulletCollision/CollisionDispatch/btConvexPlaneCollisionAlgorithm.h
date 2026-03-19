

#ifndef BT_CONVEX_PLANE_COLLISION_ALGORITHM_H
#define BT_CONVEX_PLANE_COLLISION_ALGORITHM_H

#include "BulletCollision/BroadphaseCollision/btCollisionAlgorithm.h"
#include "BulletCollision/BroadphaseCollision/btBroadphaseProxy.h"
#include "BulletCollision/CollisionDispatch/btCollisionCreateFunc.h"
class btPersistentManifold;
#include "btCollisionDispatcher.h"

#include "LinearMath/btVector3.h"



class btConvexPlaneCollisionAlgorithm : public btCollisionAlgorithm
{
	bool m_ownManifold;
	btPersistentManifold* m_manifoldPtr;
	bool m_isSwapped;
	int m_numPerturbationIterations;
	int m_minimumPointsPerturbationThreshold;

public:
	btConvexPlaneCollisionAlgorithm(btPersistentManifold* mf, const btCollisionAlgorithmConstructionInfo& ci, const btCollisionObjectWrapper* body0Wrap, const btCollisionObjectWrapper* body1Wrap, bool isSwapped, int numPerturbationIterations, int minimumPointsPerturbationThreshold);

	virtual ~btConvexPlaneCollisionAlgorithm();

	virtual void processCollision(const btCollisionObjectWrapper* body0Wrap, const btCollisionObjectWrapper* body1Wrap, const btDispatcherInfo& dispatchInfo, btManifoldResult* resultOut);

	void collideSingleContact(const btQuaternion& perturbeRot, const btCollisionObjectWrapper* body0Wrap, const btCollisionObjectWrapper* body1Wrap, const btDispatcherInfo& dispatchInfo, btManifoldResult* resultOut);

	virtual btScalar calculateTimeOfImpact(btCollisionObject* body0, btCollisionObject* body1, const btDispatcherInfo& dispatchInfo, btManifoldResult* resultOut);

	virtual void getAllContactManifolds(btManifoldArray& manifoldArray)
	{
		if (m_manifoldPtr && m_ownManifold)
		{
			manifoldArray.push_back(m_manifoldPtr);
		}
	}

	struct CreateFunc : public btCollisionAlgorithmCreateFunc
	{
		int m_numPerturbationIterations;
		int m_minimumPointsPerturbationThreshold;

		CreateFunc()
			: m_numPerturbationIterations(1),
			  m_minimumPointsPerturbationThreshold(0)
		{
		}

		virtual btCollisionAlgorithm* CreateCollisionAlgorithm(btCollisionAlgorithmConstructionInfo& ci, const btCollisionObjectWrapper* body0Wrap, const btCollisionObjectWrapper* body1Wrap)
		{
			void* mem = ci.m_dispatcher1->allocateCollisionAlgorithm(sizeof(btConvexPlaneCollisionAlgorithm));
			if (!m_swapped)
			{
				return new (mem) btConvexPlaneCollisionAlgorithm(0, ci, body0Wrap, body1Wrap, false, m_numPerturbationIterations, m_minimumPointsPerturbationThreshold);
			}
			else
			{
				return new (mem) btConvexPlaneCollisionAlgorithm(0, ci, body0Wrap, body1Wrap, true, m_numPerturbationIterations, m_minimumPointsPerturbationThreshold);
			}
		}
	};
};

#endif  
