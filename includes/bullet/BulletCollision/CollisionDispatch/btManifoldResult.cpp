

#include "btManifoldResult.h"
#include "BulletCollision/NarrowPhaseCollision/btPersistentManifold.h"
#include "BulletCollision/CollisionDispatch/btCollisionObject.h"
#include "BulletCollision/CollisionDispatch/btCollisionObjectWrapper.h"


ContactAddedCallback gContactAddedCallback = 0;

CalculateCombinedCallback gCalculateCombinedRestitutionCallback = &btManifoldResult::calculateCombinedRestitution;
CalculateCombinedCallback gCalculateCombinedFrictionCallback = &btManifoldResult::calculateCombinedFriction;
CalculateCombinedCallback gCalculateCombinedRollingFrictionCallback = &btManifoldResult::calculateCombinedRollingFriction;
CalculateCombinedCallback gCalculateCombinedSpinningFrictionCallback = &btManifoldResult::calculateCombinedSpinningFriction;
CalculateCombinedCallback gCalculateCombinedContactDampingCallback = &btManifoldResult::calculateCombinedContactDamping;
CalculateCombinedCallback gCalculateCombinedContactStiffnessCallback = &btManifoldResult::calculateCombinedContactStiffness;

btScalar btManifoldResult::calculateCombinedRollingFriction(const btCollisionObject* body0, const btCollisionObject* body1)
{
	btScalar friction = body0->getRollingFriction() * body1->getFriction() + body1->getRollingFriction() * body0->getFriction();

	const btScalar MAX_FRICTION = btScalar(10.);
	if (friction < -MAX_FRICTION)
		friction = -MAX_FRICTION;
	if (friction > MAX_FRICTION)
		friction = MAX_FRICTION;
	return friction;
}

btScalar btManifoldResult::calculateCombinedSpinningFriction(const btCollisionObject* body0, const btCollisionObject* body1)
{
	btScalar friction = body0->getSpinningFriction() * body1->getFriction() + body1->getSpinningFriction() * body0->getFriction();

	const btScalar MAX_FRICTION = btScalar(10.);
	if (friction < -MAX_FRICTION)
		friction = -MAX_FRICTION;
	if (friction > MAX_FRICTION)
		friction = MAX_FRICTION;
	return friction;
}


btScalar btManifoldResult::calculateCombinedFriction(const btCollisionObject* body0, const btCollisionObject* body1)
{
	btScalar friction = body0->getFriction() * body1->getFriction();

	const btScalar MAX_FRICTION = btScalar(10.);
	if (friction < -MAX_FRICTION)
		friction = -MAX_FRICTION;
	if (friction > MAX_FRICTION)
		friction = MAX_FRICTION;
	return friction;
}

btScalar btManifoldResult::calculateCombinedRestitution(const btCollisionObject* body0, const btCollisionObject* body1)
{
	return body0->getRestitution() * body1->getRestitution();
}

btScalar btManifoldResult::calculateCombinedContactDamping(const btCollisionObject* body0, const btCollisionObject* body1)
{
	return body0->getContactDamping() + body1->getContactDamping();
}

btScalar btManifoldResult::calculateCombinedContactStiffness(const btCollisionObject* body0, const btCollisionObject* body1)
{
	btScalar s0 = body0->getContactStiffness();
	btScalar s1 = body1->getContactStiffness();

	btScalar tmp0 = btScalar(1) / s0;
	btScalar tmp1 = btScalar(1) / s1;
	btScalar combinedStiffness = btScalar(1) / (tmp0 + tmp1);
	return combinedStiffness;
}

btManifoldResult::btManifoldResult(const btCollisionObjectWrapper* body0Wrap, const btCollisionObjectWrapper* body1Wrap)
	: m_manifoldPtr(0),
	  m_body0Wrap(body0Wrap),
	  m_body1Wrap(body1Wrap)
	  ,
	  m_partId0(-1),
	  m_partId1(-1),
	  m_index0(-1),
	  m_index1(-1)
	  ,
	  m_closestPointDistanceThreshold(0)
{
}

void btManifoldResult::addContactPoint(const btVector3& normalOnBInWorld, const btVector3& pointInWorld, btScalar depth)
{
	btAssert(m_manifoldPtr);
	

	if (depth > m_manifoldPtr->getContactBreakingThreshold())
		
		return;

	bool isSwapped = m_manifoldPtr->getBody0() != m_body0Wrap->getCollisionObject();
	bool isNewCollision = m_manifoldPtr->getNumContacts() == 0;

	btVector3 pointA = pointInWorld + normalOnBInWorld * depth;

	btVector3 localA;
	btVector3 localB;

	if (isSwapped)
	{
		localA = m_body1Wrap->getCollisionObject()->getWorldTransform().invXform(pointA);
		localB = m_body0Wrap->getCollisionObject()->getWorldTransform().invXform(pointInWorld);
	}
	else
	{
		localA = m_body0Wrap->getCollisionObject()->getWorldTransform().invXform(pointA);
		localB = m_body1Wrap->getCollisionObject()->getWorldTransform().invXform(pointInWorld);
	}

	btManifoldPoint newPt(localA, localB, normalOnBInWorld, depth);
	newPt.m_positionWorldOnA = pointA;
	newPt.m_positionWorldOnB = pointInWorld;

	int insertIndex = m_manifoldPtr->getCacheEntry(newPt);

	newPt.m_combinedFriction = gCalculateCombinedFrictionCallback(m_body0Wrap->getCollisionObject(), m_body1Wrap->getCollisionObject());
	newPt.m_combinedRestitution = gCalculateCombinedRestitutionCallback(m_body0Wrap->getCollisionObject(), m_body1Wrap->getCollisionObject());
	newPt.m_combinedRollingFriction = gCalculateCombinedRollingFrictionCallback(m_body0Wrap->getCollisionObject(), m_body1Wrap->getCollisionObject());
	newPt.m_combinedSpinningFriction = gCalculateCombinedSpinningFrictionCallback(m_body0Wrap->getCollisionObject(), m_body1Wrap->getCollisionObject());

	if ((m_body0Wrap->getCollisionObject()->getCollisionFlags() & btCollisionObject::CF_HAS_CONTACT_STIFFNESS_DAMPING) ||
		(m_body1Wrap->getCollisionObject()->getCollisionFlags() & btCollisionObject::CF_HAS_CONTACT_STIFFNESS_DAMPING))
	{
		newPt.m_combinedContactDamping1 = gCalculateCombinedContactDampingCallback(m_body0Wrap->getCollisionObject(), m_body1Wrap->getCollisionObject());
		newPt.m_combinedContactStiffness1 = gCalculateCombinedContactStiffnessCallback(m_body0Wrap->getCollisionObject(), m_body1Wrap->getCollisionObject());
		newPt.m_contactPointFlags |= BT_CONTACT_FLAG_CONTACT_STIFFNESS_DAMPING;
	}

	if ((m_body0Wrap->getCollisionObject()->getCollisionFlags() & btCollisionObject::CF_HAS_FRICTION_ANCHOR) ||
		(m_body1Wrap->getCollisionObject()->getCollisionFlags() & btCollisionObject::CF_HAS_FRICTION_ANCHOR))
	{
		newPt.m_contactPointFlags |= BT_CONTACT_FLAG_FRICTION_ANCHOR;
	}

	btPlaneSpace1(newPt.m_normalWorldOnB, newPt.m_lateralFrictionDir1, newPt.m_lateralFrictionDir2);

	
	if (isSwapped)
	{
		newPt.m_partId0 = m_partId1;
		newPt.m_partId1 = m_partId0;
		newPt.m_index0 = m_index1;
		newPt.m_index1 = m_index0;
	}
	else
	{
		newPt.m_partId0 = m_partId0;
		newPt.m_partId1 = m_partId1;
		newPt.m_index0 = m_index0;
		newPt.m_index1 = m_index1;
	}
	
	
	if (insertIndex >= 0)
	{
		
		m_manifoldPtr->replaceContactPoint(newPt, insertIndex);
	}
	else
	{
		insertIndex = m_manifoldPtr->addManifoldPoint(newPt);
	}

	
	if (gContactAddedCallback &&
		
		((m_body0Wrap->getCollisionObject()->getCollisionFlags() & btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK) ||
		 (m_body1Wrap->getCollisionObject()->getCollisionFlags() & btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK)))
	{
		
		const btCollisionObjectWrapper* obj0Wrap = isSwapped ? m_body1Wrap : m_body0Wrap;
		const btCollisionObjectWrapper* obj1Wrap = isSwapped ? m_body0Wrap : m_body1Wrap;
		(*gContactAddedCallback)(m_manifoldPtr->getContactPoint(insertIndex), obj0Wrap, newPt.m_partId0, newPt.m_index0, obj1Wrap, newPt.m_partId1, newPt.m_index1);
	}

	if (gContactStartedCallback && isNewCollision)
	{
		gContactStartedCallback(m_manifoldPtr);
	}
}
