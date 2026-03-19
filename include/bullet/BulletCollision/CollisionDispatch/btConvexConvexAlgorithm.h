

#ifndef BT_CONVEX_CONVEX_ALGORITHM_H
#define BT_CONVEX_CONVEX_ALGORITHM_H

#include "btActivatingCollisionAlgorithm.h"
#include "BulletCollision/NarrowPhaseCollision/btGjkPairDetector.h"
#include "BulletCollision/NarrowPhaseCollision/btPersistentManifold.h"
#include "BulletCollision/BroadphaseCollision/btBroadphaseProxy.h"
#include "BulletCollision/NarrowPhaseCollision/btVoronoiSimplexSolver.h"
#include "btCollisionCreateFunc.h"
#include "btCollisionDispatcher.h"
#include "LinearMath/btTransformUtil.h"  
#include "BulletCollision/NarrowPhaseCollision/btPolyhedralContactClipping.h"

class btConvexPenetrationDepthSolver;











class btConvexConvexAlgorithm : public btActivatingCollisionAlgorithm
{
#ifdef USE_SEPDISTANCE_UTIL2
	btConvexSeparatingDistanceUtil m_sepDistance;
#endif
	btConvexPenetrationDepthSolver* m_pdSolver;

	btVertexArray worldVertsB1;
	btVertexArray worldVertsB2;

	bool m_ownManifold;
	btPersistentManifold* m_manifoldPtr;
	bool m_lowLevelOfDetail;

	int m_numPerturbationIterations;
	int m_minimumPointsPerturbationThreshold;

	

public:
	btConvexConvexAlgorithm(btPersistentManifold* mf, const btCollisionAlgorithmConstructionInfo& ci, const btCollisionObjectWrapper* body0Wrap, const btCollisionObjectWrapper* body1Wrap, btConvexPenetrationDepthSolver* pdSolver, int numPerturbationIterations, int minimumPointsPerturbationThreshold);

	virtual ~btConvexConvexAlgorithm();

	virtual void processCollision(const btCollisionObjectWrapper* body0Wrap, const btCollisionObjectWrapper* body1Wrap, const btDispatcherInfo& dispatchInfo, btManifoldResult* resultOut);

	virtual btScalar calculateTimeOfImpact(btCollisionObject* body0, btCollisionObject* body1, const btDispatcherInfo& dispatchInfo, btManifoldResult* resultOut);

	virtual void getAllContactManifolds(btManifoldArray& manifoldArray)
	{
		
		if (m_manifoldPtr && m_ownManifold)
			manifoldArray.push_back(m_manifoldPtr);
	}

	void setLowLevelOfDetail(bool useLowLevel);

	const btPersistentManifold* getManifold()
	{
		return m_manifoldPtr;
	}

	struct CreateFunc : public btCollisionAlgorithmCreateFunc
	{
		btConvexPenetrationDepthSolver* m_pdSolver;
		int m_numPerturbationIterations;
		int m_minimumPointsPerturbationThreshold;

		CreateFunc(btConvexPenetrationDepthSolver* pdSolver);

		virtual ~CreateFunc();

		virtual btCollisionAlgorithm* CreateCollisionAlgorithm(btCollisionAlgorithmConstructionInfo& ci, const btCollisionObjectWrapper* body0Wrap, const btCollisionObjectWrapper* body1Wrap)
		{
			void* mem = ci.m_dispatcher1->allocateCollisionAlgorithm(sizeof(btConvexConvexAlgorithm));
			return new (mem) btConvexConvexAlgorithm(ci.m_manifold, ci, body0Wrap, body1Wrap, m_pdSolver, m_numPerturbationIterations, m_minimumPointsPerturbationThreshold);
		}
	};
};

#endif  
