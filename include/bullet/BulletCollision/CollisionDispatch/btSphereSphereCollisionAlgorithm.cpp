
#define CLEAR_MANIFOLD 1

#include "btSphereSphereCollisionAlgorithm.h"
#include "BulletCollision/CollisionDispatch/btCollisionDispatcher.h"
#include "BulletCollision/CollisionShapes/btSphereShape.h"
#include "BulletCollision/CollisionDispatch/btCollisionObject.h"
#include "BulletCollision/CollisionDispatch/btCollisionObjectWrapper.h"

btSphereSphereCollisionAlgorithm::btSphereSphereCollisionAlgorithm(btPersistentManifold* mf, const btCollisionAlgorithmConstructionInfo& ci, const btCollisionObjectWrapper* col0Wrap, const btCollisionObjectWrapper* col1Wrap)
	: btActivatingCollisionAlgorithm(ci, col0Wrap, col1Wrap),
	  m_ownManifold(false),
	  m_manifoldPtr(mf)
{
	if (!m_manifoldPtr)
	{
		m_manifoldPtr = m_dispatcher->getNewManifold(col0Wrap->getCollisionObject(), col1Wrap->getCollisionObject());
		m_ownManifold = true;
	}
}

btSphereSphereCollisionAlgorithm::~btSphereSphereCollisionAlgorithm()
{
	if (m_ownManifold)
	{
		if (m_manifoldPtr)
			m_dispatcher->releaseManifold(m_manifoldPtr);
	}
}

void btSphereSphereCollisionAlgorithm::processCollision(const btCollisionObjectWrapper* col0Wrap, const btCollisionObjectWrapper* col1Wrap, const btDispatcherInfo& dispatchInfo, btManifoldResult* resultOut)
{
	(void)dispatchInfo;

	if (!m_manifoldPtr)
		return;

	resultOut->setPersistentManifold(m_manifoldPtr);

	btSphereShape* sphere0 = (btSphereShape*)col0Wrap->getCollisionShape();
	btSphereShape* sphere1 = (btSphereShape*)col1Wrap->getCollisionShape();

	btVector3 diff = col0Wrap->getWorldTransform().getOrigin() - col1Wrap->getWorldTransform().getOrigin();
	btScalar len = diff.length();
	btScalar radius0 = sphere0->getRadius();
	btScalar radius1 = sphere1->getRadius();

#ifdef CLEAR_MANIFOLD
	m_manifoldPtr->clearManifold();  
#endif

	
	if (len > (radius0 + radius1 + resultOut->m_closestPointDistanceThreshold))
	{
#ifndef CLEAR_MANIFOLD
		resultOut->refreshContactPoints();
#endif  
		return;
	}
	
	btScalar dist = len - (radius0 + radius1);

	btVector3 normalOnSurfaceB(1, 0, 0);
	if (len > SIMD_EPSILON)
	{
		normalOnSurfaceB = diff / len;
	}

	
	
	
	btVector3 pos1 = col1Wrap->getWorldTransform().getOrigin() + radius1 * normalOnSurfaceB;

	

	resultOut->addContactPoint(normalOnSurfaceB, pos1, dist);

#ifndef CLEAR_MANIFOLD
	resultOut->refreshContactPoints();
#endif  
}

btScalar btSphereSphereCollisionAlgorithm::calculateTimeOfImpact(btCollisionObject* col0, btCollisionObject* col1, const btDispatcherInfo& dispatchInfo, btManifoldResult* resultOut)
{
	(void)col0;
	(void)col1;
	(void)dispatchInfo;
	(void)resultOut;

	
	return btScalar(1.);
}
