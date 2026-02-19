

#include "btSphereBoxCollisionAlgorithm.h"
#include "BulletCollision/CollisionDispatch/btCollisionDispatcher.h"
#include "BulletCollision/CollisionShapes/btSphereShape.h"
#include "BulletCollision/CollisionShapes/btBoxShape.h"
#include "BulletCollision/CollisionDispatch/btCollisionObject.h"
#include "BulletCollision/CollisionDispatch/btCollisionObjectWrapper.h"


btSphereBoxCollisionAlgorithm::btSphereBoxCollisionAlgorithm(btPersistentManifold* mf, const btCollisionAlgorithmConstructionInfo& ci, const btCollisionObjectWrapper* col0Wrap, const btCollisionObjectWrapper* col1Wrap, bool isSwapped)
	: btActivatingCollisionAlgorithm(ci, col0Wrap, col1Wrap),
	  m_ownManifold(false),
	  m_manifoldPtr(mf),
	  m_isSwapped(isSwapped)
{
	const btCollisionObjectWrapper* sphereObjWrap = m_isSwapped ? col1Wrap : col0Wrap;
	const btCollisionObjectWrapper* boxObjWrap = m_isSwapped ? col0Wrap : col1Wrap;

	if (!m_manifoldPtr && m_dispatcher->needsCollision(sphereObjWrap->getCollisionObject(), boxObjWrap->getCollisionObject()))
	{
		m_manifoldPtr = m_dispatcher->getNewManifold(sphereObjWrap->getCollisionObject(), boxObjWrap->getCollisionObject());
		m_ownManifold = true;
	}
}

btSphereBoxCollisionAlgorithm::~btSphereBoxCollisionAlgorithm()
{
	if (m_ownManifold)
	{
		if (m_manifoldPtr)
			m_dispatcher->releaseManifold(m_manifoldPtr);
	}
}

void btSphereBoxCollisionAlgorithm::processCollision(const btCollisionObjectWrapper* body0Wrap, const btCollisionObjectWrapper* body1Wrap, const btDispatcherInfo& dispatchInfo, btManifoldResult* resultOut)
{
	(void)dispatchInfo;
	(void)resultOut;
	if (!m_manifoldPtr)
		return;

	const btCollisionObjectWrapper* sphereObjWrap = m_isSwapped ? body1Wrap : body0Wrap;
	const btCollisionObjectWrapper* boxObjWrap = m_isSwapped ? body0Wrap : body1Wrap;

	btVector3 pOnBox;

	btVector3 normalOnSurfaceB;
	btScalar penetrationDepth;
	btVector3 sphereCenter = sphereObjWrap->getWorldTransform().getOrigin();
	const btSphereShape* sphere0 = (const btSphereShape*)sphereObjWrap->getCollisionShape();
	btScalar radius = sphere0->getRadius();
	btScalar maxContactDistance = m_manifoldPtr->getContactBreakingThreshold();

	resultOut->setPersistentManifold(m_manifoldPtr);

	if (getSphereDistance(boxObjWrap, pOnBox, normalOnSurfaceB, penetrationDepth, sphereCenter, radius, maxContactDistance))
	{
		
		resultOut->addContactPoint(normalOnSurfaceB, pOnBox, penetrationDepth);
	}

	if (m_ownManifold)
	{
		if (m_manifoldPtr->getNumContacts())
		{
			resultOut->refreshContactPoints();
		}
	}
}

btScalar btSphereBoxCollisionAlgorithm::calculateTimeOfImpact(btCollisionObject* col0, btCollisionObject* col1, const btDispatcherInfo& dispatchInfo, btManifoldResult* resultOut)
{
	(void)resultOut;
	(void)dispatchInfo;
	(void)col0;
	(void)col1;

	
	return btScalar(1.);
}

bool btSphereBoxCollisionAlgorithm::getSphereDistance(const btCollisionObjectWrapper* boxObjWrap, btVector3& pointOnBox, btVector3& normal, btScalar& penetrationDepth, const btVector3& sphereCenter, btScalar fRadius, btScalar maxContactDistance)
{
	const btBoxShape* boxShape = (const btBoxShape*)boxObjWrap->getCollisionShape();
	btVector3 const& boxHalfExtent = boxShape->getHalfExtentsWithoutMargin();
	btScalar boxMargin = boxShape->getMargin();
	penetrationDepth = 1.0f;

	
	btTransform const& m44T = boxObjWrap->getWorldTransform();
	btVector3 sphereRelPos = m44T.invXform(sphereCenter);

	
	btVector3 closestPoint = sphereRelPos;
	closestPoint.setX(btMin(boxHalfExtent.getX(), closestPoint.getX()));
	closestPoint.setX(btMax(-boxHalfExtent.getX(), closestPoint.getX()));
	closestPoint.setY(btMin(boxHalfExtent.getY(), closestPoint.getY()));
	closestPoint.setY(btMax(-boxHalfExtent.getY(), closestPoint.getY()));
	closestPoint.setZ(btMin(boxHalfExtent.getZ(), closestPoint.getZ()));
	closestPoint.setZ(btMax(-boxHalfExtent.getZ(), closestPoint.getZ()));

	btScalar intersectionDist = fRadius + boxMargin;
	btScalar contactDist = intersectionDist + maxContactDistance;
	normal = sphereRelPos - closestPoint;

	
	btScalar dist2 = normal.length2();
	if (dist2 > contactDist * contactDist)
	{
		return false;
	}

	btScalar distance;

	
	if (dist2 <= SIMD_EPSILON)
	{
		distance = -getSpherePenetration(boxHalfExtent, sphereRelPos, closestPoint, normal);
	}
	else  
	{
		distance = normal.length();
		normal /= distance;
	}

	pointOnBox = closestPoint + normal * boxMargin;
	
	penetrationDepth = distance - intersectionDist;

	
	btVector3 tmp = m44T(pointOnBox);
	pointOnBox = tmp;
	
	
	tmp = m44T.getBasis() * normal;
	normal = tmp;

	return true;
}

btScalar btSphereBoxCollisionAlgorithm::getSpherePenetration(btVector3 const& boxHalfExtent, btVector3 const& sphereRelPos, btVector3& closestPoint, btVector3& normal)
{
	
	btScalar faceDist = boxHalfExtent.getX() - sphereRelPos.getX();
	btScalar minDist = faceDist;
	closestPoint.setX(boxHalfExtent.getX());
	normal.setValue(btScalar(1.0f), btScalar(0.0f), btScalar(0.0f));

	faceDist = boxHalfExtent.getX() + sphereRelPos.getX();
	if (faceDist < minDist)
	{
		minDist = faceDist;
		closestPoint = sphereRelPos;
		closestPoint.setX(-boxHalfExtent.getX());
		normal.setValue(btScalar(-1.0f), btScalar(0.0f), btScalar(0.0f));
	}

	faceDist = boxHalfExtent.getY() - sphereRelPos.getY();
	if (faceDist < minDist)
	{
		minDist = faceDist;
		closestPoint = sphereRelPos;
		closestPoint.setY(boxHalfExtent.getY());
		normal.setValue(btScalar(0.0f), btScalar(1.0f), btScalar(0.0f));
	}

	faceDist = boxHalfExtent.getY() + sphereRelPos.getY();
	if (faceDist < minDist)
	{
		minDist = faceDist;
		closestPoint = sphereRelPos;
		closestPoint.setY(-boxHalfExtent.getY());
		normal.setValue(btScalar(0.0f), btScalar(-1.0f), btScalar(0.0f));
	}

	faceDist = boxHalfExtent.getZ() - sphereRelPos.getZ();
	if (faceDist < minDist)
	{
		minDist = faceDist;
		closestPoint = sphereRelPos;
		closestPoint.setZ(boxHalfExtent.getZ());
		normal.setValue(btScalar(0.0f), btScalar(0.0f), btScalar(1.0f));
	}

	faceDist = boxHalfExtent.getZ() + sphereRelPos.getZ();
	if (faceDist < minDist)
	{
		minDist = faceDist;
		closestPoint = sphereRelPos;
		closestPoint.setZ(-boxHalfExtent.getZ());
		normal.setValue(btScalar(0.0f), btScalar(0.0f), btScalar(-1.0f));
	}

	return minDist;
}
