

#include "btBoxBoxCollisionAlgorithm.h"
#include "BulletCollision/CollisionDispatch/btCollisionDispatcher.h"
#include "BulletCollision/CollisionShapes/btBoxShape.h"
#include "BulletCollision/CollisionDispatch/btCollisionObject.h"
#include "btBoxBoxDetector.h"
#include "BulletCollision/CollisionDispatch/btCollisionObjectWrapper.h"
#define USE_PERSISTENT_CONTACTS 1

btBoxBoxCollisionAlgorithm::btBoxBoxCollisionAlgorithm(btPersistentManifold* mf, const btCollisionAlgorithmConstructionInfo& ci, const btCollisionObjectWrapper* body0Wrap, const btCollisionObjectWrapper* body1Wrap)
	: btActivatingCollisionAlgorithm(ci, body0Wrap, body1Wrap),
	  m_ownManifold(false),
	  m_manifoldPtr(mf)
{
	if (!m_manifoldPtr && m_dispatcher->needsCollision(body0Wrap->getCollisionObject(), body1Wrap->getCollisionObject()))
	{
		m_manifoldPtr = m_dispatcher->getNewManifold(body0Wrap->getCollisionObject(), body1Wrap->getCollisionObject());
		m_ownManifold = true;
	}
}

btBoxBoxCollisionAlgorithm::~btBoxBoxCollisionAlgorithm()
{
	if (m_ownManifold)
	{
		if (m_manifoldPtr)
			m_dispatcher->releaseManifold(m_manifoldPtr);
	}
}

void btBoxBoxCollisionAlgorithm::processCollision(const btCollisionObjectWrapper* body0Wrap, const btCollisionObjectWrapper* body1Wrap, const btDispatcherInfo& dispatchInfo, btManifoldResult* resultOut)
{
	if (!m_manifoldPtr)
		return;

	const btBoxShape* box0 = (btBoxShape*)body0Wrap->getCollisionShape();
	const btBoxShape* box1 = (btBoxShape*)body1Wrap->getCollisionShape();

	
	resultOut->setPersistentManifold(m_manifoldPtr);
#ifndef USE_PERSISTENT_CONTACTS
	m_manifoldPtr->clearManifold();
#endif  

	btDiscreteCollisionDetectorInterface::ClosestPointInput input;
	input.m_maximumDistanceSquared = BT_LARGE_FLOAT;
	input.m_transformA = body0Wrap->getWorldTransform();
	input.m_transformB = body1Wrap->getWorldTransform();

	btBoxBoxDetector detector(box0, box1);
	detector.getClosestPoints(input, *resultOut, dispatchInfo.m_debugDraw);

#ifdef USE_PERSISTENT_CONTACTS
	
	if (m_ownManifold)
	{
		resultOut->refreshContactPoints();
	}
#endif  
}

btScalar btBoxBoxCollisionAlgorithm::calculateTimeOfImpact(btCollisionObject* , btCollisionObject* , const btDispatcherInfo& , btManifoldResult* )
{
	
	return 1.f;
}
