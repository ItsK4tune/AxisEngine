

#ifndef BT_EMPTY_ALGORITH
#define BT_EMPTY_ALGORITH
#include "BulletCollision/BroadphaseCollision/btCollisionAlgorithm.h"
#include "btCollisionCreateFunc.h"
#include "btCollisionDispatcher.h"

#define ATTRIBUTE_ALIGNED(a)



class btEmptyAlgorithm : public btCollisionAlgorithm
{
public:
	btEmptyAlgorithm(const btCollisionAlgorithmConstructionInfo& ci);

	virtual void processCollision(const btCollisionObjectWrapper* body0Wrap, const btCollisionObjectWrapper* body1Wrap, const btDispatcherInfo& dispatchInfo, btManifoldResult* resultOut);

	virtual btScalar calculateTimeOfImpact(btCollisionObject* body0, btCollisionObject* body1, const btDispatcherInfo& dispatchInfo, btManifoldResult* resultOut);

	virtual void getAllContactManifolds(btManifoldArray& manifoldArray)
	{
	}

	struct CreateFunc : public btCollisionAlgorithmCreateFunc
	{
		virtual btCollisionAlgorithm* CreateCollisionAlgorithm(btCollisionAlgorithmConstructionInfo& ci, const btCollisionObjectWrapper* body0Wrap, const btCollisionObjectWrapper* body1Wrap)
		{
			(void)body0Wrap;
			(void)body1Wrap;
			void* mem = ci.m_dispatcher1->allocateCollisionAlgorithm(sizeof(btEmptyAlgorithm));
			return new (mem) btEmptyAlgorithm(ci);
		}
	};

} ATTRIBUTE_ALIGNED(16);

#endif  
