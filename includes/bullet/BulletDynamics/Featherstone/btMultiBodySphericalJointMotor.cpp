



#include "btMultiBodySphericalJointMotor.h"
#include "btMultiBody.h"
#include "btMultiBodyLinkCollider.h"
#include "BulletCollision/CollisionDispatch/btCollisionObject.h"
#include "LinearMath/btTransformUtil.h"
#include "BulletDynamics/ConstraintSolver/btGeneric6DofSpring2Constraint.h"

btMultiBodySphericalJointMotor::btMultiBodySphericalJointMotor(btMultiBody* body, int link, btScalar maxMotorImpulse)
	: btMultiBodyConstraint(body, body, link, body->getLink(link).m_parent, 3, true, MULTIBODY_CONSTRAINT_SPHERICAL_MOTOR),
	m_desiredVelocity(0, 0, 0),
	m_desiredPosition(0,0,0,1),
	m_use_multi_dof_params(false),
	m_kd(1., 1., 1.),
	m_kp(0.2, 0.2, 0.2),
	m_erp(1),
	m_rhsClamp(SIMD_INFINITY),
	m_maxAppliedImpulseMultiDof(maxMotorImpulse, maxMotorImpulse, maxMotorImpulse),
	m_damping(1.0, 1.0, 1.0)
{

	m_maxAppliedImpulse = maxMotorImpulse;
}


void btMultiBodySphericalJointMotor::finalizeMultiDof()
{
	allocateJacobiansMultiDof();
	
	
	int linkDoF = 0;
	unsigned int offset = 6 + (m_bodyA->getLink(m_linkA).m_dofOffset + linkDoF);

	
	
	jacobianA(0)[offset] = 1;

	m_numDofsFinalized = m_jacSizeBoth;
}


btMultiBodySphericalJointMotor::~btMultiBodySphericalJointMotor()
{
}

int btMultiBodySphericalJointMotor::getIslandIdA() const
{
	if (this->m_linkA < 0)
	{
		btMultiBodyLinkCollider* col = m_bodyA->getBaseCollider();
		if (col)
			return col->getIslandTag();
	}
	else
	{
		if (m_bodyA->getLink(m_linkA).m_collider)
		{
			return m_bodyA->getLink(m_linkA).m_collider->getIslandTag();
		}
	}
	return -1;
}

int btMultiBodySphericalJointMotor::getIslandIdB() const
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
		{
			return m_bodyB->getLink(m_linkB).m_collider->getIslandTag();
		}
	}
	return -1;
}

void btMultiBodySphericalJointMotor::createConstraintRows(btMultiBodyConstraintArray& constraintRows,
												 btMultiBodyJacobianData& data,
												 const btContactSolverInfo& infoGlobal)
{
	
	

	if (m_numDofsFinalized != m_jacSizeBoth)
	{
		finalizeMultiDof();
	}

	
	if (m_numDofsFinalized != m_jacSizeBoth)
		return;
	

	if (m_maxAppliedImpulse == 0.f)
		return;

	const btScalar posError = 0;
	const btVector3 dummy(0, 0, 0);

	
	btVector3 axis[3] = { btVector3(1, 0, 0), btVector3(0, 1, 0), btVector3(0, 0, 1) };
	
	btQuaternion desiredQuat = m_desiredPosition;
	btQuaternion currentQuat(m_bodyA->getJointPosMultiDof(m_linkA)[0],
		m_bodyA->getJointPosMultiDof(m_linkA)[1],
		m_bodyA->getJointPosMultiDof(m_linkA)[2],
		m_bodyA->getJointPosMultiDof(m_linkA)[3]);

btQuaternion relRot = currentQuat.inverse() * desiredQuat;
	btVector3 angleDiff;
	btGeneric6DofSpring2Constraint::matrixToEulerXYZ(btMatrix3x3(relRot), angleDiff);



	for (int row = 0; row < getNumRows(); row++)
	{
		btMultiBodySolverConstraint& constraintRow = constraintRows.expandNonInitializing();

		int dof = row;
		
		btScalar currentVelocity = m_bodyA->getJointVelMultiDof(m_linkA)[dof];
		btScalar desiredVelocity = this->m_desiredVelocity[row];
		
		double kd = m_use_multi_dof_params ? m_kd[row % 3] : m_kd[0];
		btScalar velocityError = (desiredVelocity - currentVelocity) * kd;

		btMatrix3x3 frameAworld;
		frameAworld.setIdentity();
		frameAworld = m_bodyA->localFrameToWorld(m_linkA, frameAworld);
		btScalar posError = 0;
		{
			btAssert(m_bodyA->getLink(m_linkA).m_jointType == btMultibodyLink::eSpherical);
			switch (m_bodyA->getLink(m_linkA).m_jointType)
			{
				case btMultibodyLink::eSpherical:
				{
					btVector3 constraintNormalAng = frameAworld.getColumn(row % 3);
					double kp = m_use_multi_dof_params ? m_kp[row % 3] : m_kp[0];
					posError = kp*angleDiff[row % 3];
					double max_applied_impulse = m_use_multi_dof_params ? m_maxAppliedImpulseMultiDof[row % 3] : m_maxAppliedImpulse;
					fillMultiBodyConstraint(constraintRow, data, 0, 0, constraintNormalAng,
						btVector3(0,0,0), dummy, dummy,
						posError,
						infoGlobal,
						-max_applied_impulse, max_applied_impulse, true,
						1.0, false, 0, 0,
						m_damping[row % 3]);
					constraintRow.m_orgConstraint = this;
					constraintRow.m_orgDofIndex = row;
					break;
				}
				default:
				{
					btAssert(0);
				}
			};
		}
	}
}
