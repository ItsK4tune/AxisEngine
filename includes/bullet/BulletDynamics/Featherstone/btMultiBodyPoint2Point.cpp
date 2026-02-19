



#include "btMultiBodyPoint2Point.h"
#include "btMultiBodyLinkCollider.h"
#include "BulletDynamics/Dynamics/btRigidBody.h"
#include "LinearMath/btIDebugDraw.h"

#ifndef BTMBP2PCONSTRAINT_BLOCK_ANGULAR_MOTION_TEST
#define BTMBP2PCONSTRAINT_DIM 3
#else
#define BTMBP2PCONSTRAINT_DIM 6
#endif

btMultiBodyPoint2Point::btMultiBodyPoint2Point(btMultiBody* body, int link, btRigidBody* bodyB, const btVector3& pivotInA, const btVector3& pivotInB)
	: btMultiBodyConstraint(body, 0, link, -1, BTMBP2PCONSTRAINT_DIM, false, MULTIBODY_CONSTRAINT_POINT_TO_POINT),
	  m_rigidBodyA(0),
	  m_rigidBodyB(bodyB),
	  m_pivotInA(pivotInA),
	  m_pivotInB(pivotInB)
{
	m_data.resize(BTMBP2PCONSTRAINT_DIM);  
}

btMultiBodyPoint2Point::btMultiBodyPoint2Point(btMultiBody* bodyA, int linkA, btMultiBody* bodyB, int linkB, const btVector3& pivotInA, const btVector3& pivotInB)
	: btMultiBodyConstraint(bodyA, bodyB, linkA, linkB, BTMBP2PCONSTRAINT_DIM, false, MULTIBODY_CONSTRAINT_POINT_TO_POINT),
	  m_rigidBodyA(0),
	  m_rigidBodyB(0),
	  m_pivotInA(pivotInA),
	  m_pivotInB(pivotInB)
{
	m_data.resize(BTMBP2PCONSTRAINT_DIM);  
}

void btMultiBodyPoint2Point::finalizeMultiDof()
{
	
	btAssert(0);
}

btMultiBodyPoint2Point::~btMultiBodyPoint2Point()
{
}

int btMultiBodyPoint2Point::getIslandIdA() const
{
	if (m_rigidBodyA)
		return m_rigidBodyA->getIslandTag();

	if (m_bodyA)
	{
		if (m_linkA < 0)
		{
			btMultiBodyLinkCollider* col = m_bodyA->getBaseCollider();
			if (col)
				return col->getIslandTag();
		}
		else
		{
			if (m_bodyA->getLink(m_linkA).m_collider)
				return m_bodyA->getLink(m_linkA).m_collider->getIslandTag();
		}
	}
	return -1;
}

int btMultiBodyPoint2Point::getIslandIdB() const
{
	if (m_rigidBodyB)
		return m_rigidBodyB->getIslandTag();
	if (m_bodyB)
	{
		if (m_linkB < 0)
		{
			btMultiBodyLinkCollider* col = m_bodyB->getBaseCollider();
			if (col)
				return col->getIslandTag();
		}
		else
		{
			if (m_bodyB->getLink(m_linkB).m_collider)
				return m_bodyB->getLink(m_linkB).m_collider->getIslandTag();
		}
	}
	return -1;
}

void btMultiBodyPoint2Point::createConstraintRows(btMultiBodyConstraintArray& constraintRows,
												  btMultiBodyJacobianData& data,
												  const btContactSolverInfo& infoGlobal)
{
	
	int numDim = BTMBP2PCONSTRAINT_DIM;
	for (int i = 0; i < numDim; i++)
	{
		btMultiBodySolverConstraint& constraintRow = constraintRows.expandNonInitializing();
		
		constraintRow.m_orgConstraint = this;
		constraintRow.m_orgDofIndex = i;
		constraintRow.m_relpos1CrossNormal.setValue(0, 0, 0);
		constraintRow.m_contactNormal1.setValue(0, 0, 0);
		constraintRow.m_relpos2CrossNormal.setValue(0, 0, 0);
		constraintRow.m_contactNormal2.setValue(0, 0, 0);
		constraintRow.m_angularComponentA.setValue(0, 0, 0);
		constraintRow.m_angularComponentB.setValue(0, 0, 0);

		constraintRow.m_solverBodyIdA = data.m_fixedBodyId;
		constraintRow.m_solverBodyIdB = data.m_fixedBodyId;

		btVector3 contactNormalOnB(0, 0, 0);
#ifndef BTMBP2PCONSTRAINT_BLOCK_ANGULAR_MOTION_TEST
		contactNormalOnB[i] = -1;
#else
		contactNormalOnB[i % 3] = -1;
#endif

		
		btVector3 pivotAworld = m_pivotInA;
		if (m_rigidBodyA)
		{
			constraintRow.m_solverBodyIdA = m_rigidBodyA->getCompanionId();
			pivotAworld = m_rigidBodyA->getCenterOfMassTransform() * m_pivotInA;
		}
		else
		{
			if (m_bodyA)
				pivotAworld = m_bodyA->localPosToWorld(m_linkA, m_pivotInA);
		}
		btVector3 pivotBworld = m_pivotInB;
		if (m_rigidBodyB)
		{
			constraintRow.m_solverBodyIdB = m_rigidBodyB->getCompanionId();
			pivotBworld = m_rigidBodyB->getCenterOfMassTransform() * m_pivotInB;
		}
		else
		{
			if (m_bodyB)
				pivotBworld = m_bodyB->localPosToWorld(m_linkB, m_pivotInB);
		}

		btScalar posError = i < 3 ? (pivotAworld - pivotBworld).dot(contactNormalOnB) : 0;

#ifndef BTMBP2PCONSTRAINT_BLOCK_ANGULAR_MOTION_TEST

		fillMultiBodyConstraint(constraintRow, data, 0, 0, btVector3(0, 0, 0),
								contactNormalOnB, pivotAworld, pivotBworld,  
								posError,
								infoGlobal,
								-m_maxAppliedImpulse, m_maxAppliedImpulse);
		
		
#else
		const btVector3 dummy(0, 0, 0);

		btAssert(m_bodyA->isMultiDof());

		btScalar* jac1 = jacobianA(i);
		const btVector3& normalAng = i >= 3 ? contactNormalOnB : dummy;
		const btVector3& normalLin = i < 3 ? contactNormalOnB : dummy;

		m_bodyA->filConstraintJacobianMultiDof(m_linkA, pivotAworld, normalAng, normalLin, jac1, data.scratch_r, data.scratch_v, data.scratch_m);

		fillMultiBodyConstraint(constraintRow, data, jac1, 0,
								dummy, dummy, dummy,  
								posError,
								infoGlobal,
								-m_maxAppliedImpulse, m_maxAppliedImpulse);
#endif
	}
}

void btMultiBodyPoint2Point::debugDraw(class btIDebugDraw* drawer)
{
	btTransform tr;
	tr.setIdentity();

	if (m_rigidBodyA)
	{
		btVector3 pivot = m_rigidBodyA->getCenterOfMassTransform() * m_pivotInA;
		tr.setOrigin(pivot);
		drawer->drawTransform(tr, 0.1);
	}
	if (m_bodyA)
	{
		btVector3 pivotAworld = m_bodyA->localPosToWorld(m_linkA, m_pivotInA);
		tr.setOrigin(pivotAworld);
		drawer->drawTransform(tr, 0.1);
	}
	if (m_rigidBodyB)
	{
		
		btVector3 pivot = m_rigidBodyB->getCenterOfMassTransform() * m_pivotInB;
		tr.setOrigin(pivot);
		drawer->drawTransform(tr, 0.1);
	}
	if (m_bodyB)
	{
		btVector3 pivotBworld = m_bodyB->localPosToWorld(m_linkB, m_pivotInB);
		tr.setOrigin(pivotBworld);
		drawer->drawTransform(tr, 0.1);
	}
}
