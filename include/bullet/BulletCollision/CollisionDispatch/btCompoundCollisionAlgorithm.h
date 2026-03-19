

#ifndef BT_COMPOUND_COLLISION_ALGORITHM_H
#define BT_COMPOUND_COLLISION_ALGORITHM_H

#include "btActivatingCollisionAlgorithm.h"
#include "BulletCollision/BroadphaseCollision/btDispatcher.h"
#include "BulletCollision/BroadphaseCollision/btBroadphaseInterface.h"

#include "BulletCollision/NarrowPhaseCollision/btPersistentManifold.h"
class btDispatcher;
#include "BulletCollision/BroadphaseCollision/btBroadphaseProxy.h"
#include "btCollisionCreateFunc.h"
#include "LinearMath/btAlignedObjectArray.h"
#include "BulletCollision/BroadphaseCollision/btDbvt.h"
class btDispatcher;
class btCollisionObject;

class btCollisionShape;
typedef bool (*btShapePairCallback)(const btCollisionShape* pShape0, const btCollisionShape* pShape1);
extern btShapePairCallback gCompoundChildShapePairCallback;


class btCompoundCollisionAlgorithm : public btActivatingCollisionAlgorithm
{
	btNodeStack stack2;
	btManifoldArray manifoldArray;

protected:
	btAlignedObjectArray<btCollisionAlgorithm*> m_childCollisionAlgorithms;
	bool m_isSwapped;

	class btPersistentManifold* m_sharedManifold;
	bool m_ownsManifold;

	int m_compoundShapeRevision;  

	void removeChildAlgorithms();

	void preallocateChildAlgorithms(const btCollisionObjectWrapper* body0Wrap, const btCollisionObjectWrapper* body1Wrap);

public:
	btCompoundCollisionAlgorithm(const btCollisionAlgorithmConstructionInfo& ci, const btCollisionObjectWrapper* body0Wrap, const btCollisionObjectWrapper* body1Wrap, bool isSwapped);

	virtual ~btCompoundCollisionAlgorithm();

	btCollisionAlgorithm* getChildAlgorithm(int n) const
	{
		return m_childCollisionAlgorithms[n];
	}

	virtual void processCollision(const btCollisionObjectWrapper* body0Wrap, const btCollisionObjectWrapper* body1Wrap, const btDispatcherInfo& dispatchInfo, btManifoldResult* resultOut);

	btScalar calculateTimeOfImpact(btCollisionObject* body0, btCollisionObject* body1, const btDispatcherInfo& dispatchInfo, btManifoldResult* resultOut);

	virtual void getAllContactManifolds(btManifoldArray& manifoldArray)
	{
		int i;
		for (i = 0; i < m_childCollisionAlgorithms.size(); i++)
		{
			if (m_childCollisionAlgorithms[i])
				m_childCollisionAlgorithms[i]->getAllContactManifolds(manifoldArray);
		}
	}

	struct CreateFunc : public btCollisionAlgorithmCreateFunc
	{
		virtual btCollisionAlgorithm* CreateCollisionAlgorithm(btCollisionAlgorithmConstructionInfo& ci, const btCollisionObjectWrapper* body0Wrap, const btCollisionObjectWrapper* body1Wrap)
		{
			void* mem = ci.m_dispatcher1->allocateCollisionAlgorithm(sizeof(btCompoundCollisionAlgorithm));
			return new (mem) btCompoundCollisionAlgorithm(ci, body0Wrap, body1Wrap, false);
		}
	};

	struct SwappedCreateFunc : public btCollisionAlgorithmCreateFunc
	{
		virtual btCollisionAlgorithm* CreateCollisionAlgorithm(btCollisionAlgorithmConstructionInfo& ci, const btCollisionObjectWrapper* body0Wrap, const btCollisionObjectWrapper* body1Wrap)
		{
			void* mem = ci.m_dispatcher1->allocateCollisionAlgorithm(sizeof(btCompoundCollisionAlgorithm));
			return new (mem) btCompoundCollisionAlgorithm(ci, body0Wrap, body1Wrap, true);
		}
	};
};

#endif  
