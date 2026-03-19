

#include "btConvexPlaneCollisionAlgorithm.h"

#include "BulletCollision/CollisionDispatch/btCollisionDispatcher.h"
#include "BulletCollision/CollisionDispatch/btCollisionObject.h"
#include "BulletCollision/CollisionShapes/btConvexShape.h"
#include "BulletCollision/CollisionShapes/btStaticPlaneShape.h"
#include "BulletCollision/CollisionDispatch/btCollisionObjectWrapper.h"



btConvexPlaneCollisionAlgorithm::btConvexPlaneCollisionAlgorithm(btPersistentManifold* mf, const btCollisionAlgorithmConstructionInfo& ci, const btCollisionObjectWrapper* col0Wrap, const btCollisionObjectWrapper* col1Wrap, bool isSwapped, int numPerturbationIterations, int minimumPointsPerturbationThreshold)
	: btCollisionAlgorithm(ci),
	  m_ownManifold(false),
	  m_manifoldPtr(mf),
	  m_isSwapped(isSwapped),
	  m_numPerturbationIterations(numPerturbationIterations),
	  m_minimumPointsPerturbationThreshold(minimumPointsPerturbationThreshold)
{
	const btCollisionObjectWrapper* convexObjWrap = m_isSwapped ? col1Wrap : col0Wrap;
	const btCollisionObjectWrapper* planeObjWrap = m_isSwapped ? col0Wrap : col1Wrap;

	if (!m_manifoldPtr && m_dispatcher->needsCollision(convexObjWrap->getCollisionObject(), planeObjWrap->getCollisionObject()))
	{
		m_manifoldPtr = m_dispatcher->getNewManifold(convexObjWrap->getCollisionObject(), planeObjWrap->getCollisionObject());
		m_ownManifold = true;
	}
}

btConvexPlaneCollisionAlgorithm::~btConvexPlaneCollisionAlgorithm()
{
	if (m_ownManifold)
	{
		if (m_manifoldPtr)
			m_dispatcher->releaseManifold(m_manifoldPtr);
	}
}

void btConvexPlaneCollisionAlgorithm::collideSingleContact(const btQuaternion& perturbeRot, const btCollisionObjectWrapper* body0Wrap, const btCollisionObjectWrapper* body1Wrap, const btDispatcherInfo& dispatchInfo, btManifoldResult* resultOut)
{
	const btCollisionObjectWrapper* convexObjWrap = m_isSwapped ? body1Wrap : body0Wrap;
	const btCollisionObjectWrapper* planeObjWrap = m_isSwapped ? body0Wrap : body1Wrap;

	btConvexShape* convexShape = (btConvexShape*)convexObjWrap->getCollisionShape();
	btStaticPlaneShape* planeShape = (btStaticPlaneShape*)planeObjWrap->getCollisionShape();

	bool hasCollision = false;
	const btVector3& planeNormal = planeShape->getPlaneNormal();
	const btScalar& planeConstant = planeShape->getPlaneConstant();

	btTransform convexWorldTransform = convexObjWrap->getWorldTransform();
	btTransform convexInPlaneTrans;
	convexInPlaneTrans = planeObjWrap->getWorldTransform().inverse() * convexWorldTransform;
	
	convexWorldTransform.getBasis() *= btMatrix3x3(perturbeRot);
	btTransform planeInConvex;
	planeInConvex = convexWorldTransform.inverse() * planeObjWrap->getWorldTransform();

	btVector3 vtx = convexShape->localGetSupportingVertex(planeInConvex.getBasis() * -planeNormal);

	btVector3 vtxInPlane = convexInPlaneTrans(vtx);
	btScalar distance = (planeNormal.dot(vtxInPlane) - planeConstant);

	btVector3 vtxInPlaneProjected = vtxInPlane - distance * planeNormal;
	btVector3 vtxInPlaneWorld = planeObjWrap->getWorldTransform() * vtxInPlaneProjected;

	hasCollision = distance < m_manifoldPtr->getContactBreakingThreshold();
	resultOut->setPersistentManifold(m_manifoldPtr);
	if (hasCollision)
	{
		
		btVector3 normalOnSurfaceB = planeObjWrap->getWorldTransform().getBasis() * planeNormal;
		btVector3 pOnB = vtxInPlaneWorld;
		resultOut->addContactPoint(normalOnSurfaceB, pOnB, distance);
	}
}

void btConvexPlaneCollisionAlgorithm::processCollision(const btCollisionObjectWrapper* body0Wrap, const btCollisionObjectWrapper* body1Wrap, const btDispatcherInfo& dispatchInfo, btManifoldResult* resultOut)
{
	(void)dispatchInfo;
	if (!m_manifoldPtr)
		return;

	const btCollisionObjectWrapper* convexObjWrap = m_isSwapped ? body1Wrap : body0Wrap;
	const btCollisionObjectWrapper* planeObjWrap = m_isSwapped ? body0Wrap : body1Wrap;

	btConvexShape* convexShape = (btConvexShape*)convexObjWrap->getCollisionShape();
	btStaticPlaneShape* planeShape = (btStaticPlaneShape*)planeObjWrap->getCollisionShape();

	bool hasCollision = false;
	const btVector3& planeNormal = planeShape->getPlaneNormal();
	const btScalar& planeConstant = planeShape->getPlaneConstant();
	btTransform planeInConvex;
	planeInConvex = convexObjWrap->getWorldTransform().inverse() * planeObjWrap->getWorldTransform();
	btTransform convexInPlaneTrans;
	convexInPlaneTrans = planeObjWrap->getWorldTransform().inverse() * convexObjWrap->getWorldTransform();

	btVector3 vtx = convexShape->localGetSupportingVertex(planeInConvex.getBasis() * -planeNormal);
	btVector3 vtxInPlane = convexInPlaneTrans(vtx);
	btScalar distance = (planeNormal.dot(vtxInPlane) - planeConstant);

	btVector3 vtxInPlaneProjected = vtxInPlane - distance * planeNormal;
	btVector3 vtxInPlaneWorld = planeObjWrap->getWorldTransform() * vtxInPlaneProjected;

	hasCollision = distance < m_manifoldPtr->getContactBreakingThreshold()+ resultOut->m_closestPointDistanceThreshold;
	resultOut->setPersistentManifold(m_manifoldPtr);
	if (hasCollision)
	{
		
		btVector3 normalOnSurfaceB = planeObjWrap->getWorldTransform().getBasis() * planeNormal;
		btVector3 pOnB = vtxInPlaneWorld;
		resultOut->addContactPoint(normalOnSurfaceB, pOnB, distance);
	}

	
	
	
	if (convexShape->isPolyhedral() && resultOut->getPersistentManifold()->getNumContacts() < m_minimumPointsPerturbationThreshold)
	{
		btVector3 v0, v1;
		btPlaneSpace1(planeNormal, v0, v1);
		

		const btScalar angleLimit = 0.125f * SIMD_PI;
		btScalar perturbeAngle;
		btScalar radius = convexShape->getAngularMotionDisc();
		perturbeAngle = gContactBreakingThreshold / radius;
		if (perturbeAngle > angleLimit)
			perturbeAngle = angleLimit;

		btQuaternion perturbeRot(v0, perturbeAngle);
		for (int i = 0; i < m_numPerturbationIterations; i++)
		{
			btScalar iterationAngle = i * (SIMD_2_PI / btScalar(m_numPerturbationIterations));
			btQuaternion rotq(planeNormal, iterationAngle);
			collideSingleContact(rotq.inverse() * perturbeRot * rotq, body0Wrap, body1Wrap, dispatchInfo, resultOut);
		}
	}

	if (m_ownManifold)
	{
		if (m_manifoldPtr->getNumContacts())
		{
			resultOut->refreshContactPoints();
		}
	}
}

btScalar btConvexPlaneCollisionAlgorithm::calculateTimeOfImpact(btCollisionObject* col0, btCollisionObject* col1, const btDispatcherInfo& dispatchInfo, btManifoldResult* resultOut)
{
	(void)resultOut;
	(void)dispatchInfo;
	(void)col0;
	(void)col1;

	
	return btScalar(1.);
}
