



#include "btMultiBodyJointLimitConstraint.h"
#include "btMultiBody.h"
#include "btMultiBodyLinkCollider.h"
#include "BulletCollision/CollisionDispatch/btCollisionObject.h"

btMultiBodyJointLimitConstraint::btMultiBodyJointLimitConstraint(btMultiBody* body, int link, btScalar lower, btScalar upper)
	
	: btMultiBodyConstraint(body, body, link, body->getLink(link).m_parent, 2, true, MULTIBODY_CONSTRAINT_LIMIT),
	  m_lowerBound(lower),
	  m_upperBound(upper)
{
}

void btMultiBodyJointLimitConstraint::finalizeMultiDof()
{
	
	

	allocateJacobiansMultiDof();

	unsigned int offset = 6 + m_bodyA->getLink(m_linkA).m_dofOffset;

	
	jacobianA(0)[offset] = 1;
	
	
	jacobianB(1)[offset] = -1;

	m_numDofsFinalized = m_jacSizeBoth;
}

btMultiBodyJointLimitConstraint::~btMultiBodyJointLimitConstraint()
{
}

int btMultiBodyJointLimitConstraint::getIslandIdA() const
{
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

int btMultiBodyJointLimitConstraint::getIslandIdB() const
{
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

void btMultiBodyJointLimitConstraint::createConstraintRows(btMultiBodyConstraintArray& constraintRows,
														   btMultiBodyJacobianData& data,
														   const btContactSolverInfo& infoGlobal)
{
	
	

	if (m_numDofsFinalized != m_jacSizeBoth)
	{
		finalizeMultiDof();
	}

	
	setPosition(0, m_bodyA->getJointPos(m_linkA) - m_lowerBound);  

	
	setPosition(1, m_upperBound - m_bodyA->getJointPos(m_linkA));

	for (int row = 0; row < getNumRows(); row++)
	{
		btScalar penetration = getPosition(row);

		
		if (penetration > 0)
		{
			continue;
		}
		btScalar direction = row ? -1 : 1;

		btMultiBodySolverConstraint& constraintRow = constraintRows.expandNonInitializing();
		constraintRow.m_orgConstraint = this;
		constraintRow.m_orgDofIndex = row;

		constraintRow.m_multiBodyA = m_bodyA;
		constraintRow.m_multiBodyB = m_bodyB;
		const btScalar posError = 0;  
		const btVector3 dummy(0, 0, 0);

		btScalar rel_vel = fillMultiBodyConstraint(constraintRow, data, jacobianA(row), jacobianB(row), dummy, dummy, dummy, dummy, posError, infoGlobal, 0, m_maxAppliedImpulse);

		{
			
			btAssert((m_bodyA->getLink(m_linkA).m_jointType == btMultibodyLink::eRevolute) || (m_bodyA->getLink(m_linkA).m_jointType == btMultibodyLink::ePrismatic));
			switch (m_bodyA->getLink(m_linkA).m_jointType)
			{
				case btMultibodyLink::eRevolute:
				{
					constraintRow.m_contactNormal1.setZero();
					constraintRow.m_contactNormal2.setZero();
					btVector3 revoluteAxisInWorld = direction * quatRotate(m_bodyA->getLink(m_linkA).m_cachedWorldTransform.getRotation(), m_bodyA->getLink(m_linkA).m_axes[0].m_topVec);
					constraintRow.m_relpos1CrossNormal = revoluteAxisInWorld;
					constraintRow.m_relpos2CrossNormal = -revoluteAxisInWorld;

					break;
				}
				case btMultibodyLink::ePrismatic:
				{
					btVector3 prismaticAxisInWorld = direction * quatRotate(m_bodyA->getLink(m_linkA).m_cachedWorldTransform.getRotation(), m_bodyA->getLink(m_linkA).m_axes[0].m_bottomVec);
					constraintRow.m_contactNormal1 = prismaticAxisInWorld;
					constraintRow.m_contactNormal2 = -prismaticAxisInWorld;
					constraintRow.m_relpos1CrossNormal.setZero();
					constraintRow.m_relpos2CrossNormal.setZero();

					break;
				}
				default:
				{
					btAssert(0);
				}
			};
		}

		{
			btScalar positionalError = 0.f;
			btScalar velocityError = -rel_vel;  
			btScalar erp = infoGlobal.m_erp2;
			if (!infoGlobal.m_splitImpulse || (penetration > infoGlobal.m_splitImpulsePenetrationThreshold))
			{
				erp = infoGlobal.m_erp;
			}
			if (penetration > 0)
			{
				positionalError = 0;
				velocityError = -penetration / infoGlobal.m_timeStep;
			}
			else
			{
				positionalError = -penetration * erp / infoGlobal.m_timeStep;
			}

			btScalar penetrationImpulse = positionalError * constraintRow.m_jacDiagABInv;
			btScalar velocityImpulse = velocityError * constraintRow.m_jacDiagABInv;
			if (!infoGlobal.m_splitImpulse || (penetration > infoGlobal.m_splitImpulsePenetrationThreshold))
			{
				
				constraintRow.m_rhs = penetrationImpulse + velocityImpulse;
				constraintRow.m_rhsPenetration = 0.f;
			}
			else
			{
				
				constraintRow.m_rhs = velocityImpulse;
				constraintRow.m_rhsPenetration = penetrationImpulse;
			}
		}
	}
}
