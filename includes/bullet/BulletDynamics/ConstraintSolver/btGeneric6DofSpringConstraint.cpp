

#include "btGeneric6DofSpringConstraint.h"
#include "BulletDynamics/Dynamics/btRigidBody.h"
#include "LinearMath/btTransformUtil.h"

btGeneric6DofSpringConstraint::btGeneric6DofSpringConstraint(btRigidBody& rbA, btRigidBody& rbB, const btTransform& frameInA, const btTransform& frameInB, bool useLinearReferenceFrameA)
	: btGeneric6DofConstraint(rbA, rbB, frameInA, frameInB, useLinearReferenceFrameA)
{
	init();
}

btGeneric6DofSpringConstraint::btGeneric6DofSpringConstraint(btRigidBody& rbB, const btTransform& frameInB, bool useLinearReferenceFrameB)
	: btGeneric6DofConstraint(rbB, frameInB, useLinearReferenceFrameB)
{
	init();
}

void btGeneric6DofSpringConstraint::init()
{
	m_objectType = D6_SPRING_CONSTRAINT_TYPE;

	for (int i = 0; i < 6; i++)
	{
		m_springEnabled[i] = false;
		m_equilibriumPoint[i] = btScalar(0.f);
		m_springStiffness[i] = btScalar(0.f);
		m_springDamping[i] = btScalar(1.f);
	}
}

void btGeneric6DofSpringConstraint::enableSpring(int index, bool onOff)
{
	btAssert((index >= 0) && (index < 6));
	m_springEnabled[index] = onOff;
	if (index < 3)
	{
		m_linearLimits.m_enableMotor[index] = onOff;
	}
	else
	{
		m_angularLimits[index - 3].m_enableMotor = onOff;
	}
}

void btGeneric6DofSpringConstraint::setStiffness(int index, btScalar stiffness)
{
	btAssert((index >= 0) && (index < 6));
	m_springStiffness[index] = stiffness;
}

void btGeneric6DofSpringConstraint::setDamping(int index, btScalar damping)
{
	btAssert((index >= 0) && (index < 6));
	m_springDamping[index] = damping;
}

void btGeneric6DofSpringConstraint::setEquilibriumPoint()
{
	calculateTransforms();
	int i;

	for (i = 0; i < 3; i++)
	{
		m_equilibriumPoint[i] = m_calculatedLinearDiff[i];
	}
	for (i = 0; i < 3; i++)
	{
		m_equilibriumPoint[i + 3] = m_calculatedAxisAngleDiff[i];
	}
}

void btGeneric6DofSpringConstraint::setEquilibriumPoint(int index)
{
	btAssert((index >= 0) && (index < 6));
	calculateTransforms();
	if (index < 3)
	{
		m_equilibriumPoint[index] = m_calculatedLinearDiff[index];
	}
	else
	{
		m_equilibriumPoint[index] = m_calculatedAxisAngleDiff[index - 3];
	}
}

void btGeneric6DofSpringConstraint::setEquilibriumPoint(int index, btScalar val)
{
	btAssert((index >= 0) && (index < 6));
	m_equilibriumPoint[index] = val;
}

void btGeneric6DofSpringConstraint::internalUpdateSprings(btConstraintInfo2* info)
{
	
	int i;
	
	for (i = 0; i < 3; i++)
	{
		if (m_springEnabled[i])
		{
			
			btScalar currPos = m_calculatedLinearDiff[i];
			
			btScalar delta = currPos - m_equilibriumPoint[i];
			
			btScalar force = delta * m_springStiffness[i];
			btScalar velFactor = info->fps * m_springDamping[i] / btScalar(info->m_numIterations);
			m_linearLimits.m_targetVelocity[i] = velFactor * force;
			m_linearLimits.m_maxMotorForce[i] = btFabs(force);
		}
	}
	for (i = 0; i < 3; i++)
	{
		if (m_springEnabled[i + 3])
		{
			
			btScalar currPos = m_calculatedAxisAngleDiff[i];
			
			btScalar delta = currPos - m_equilibriumPoint[i + 3];
			
			btScalar force = -delta * m_springStiffness[i + 3];
			btScalar velFactor = info->fps * m_springDamping[i + 3] / btScalar(info->m_numIterations);
			m_angularLimits[i].m_targetVelocity = velFactor * force;
			m_angularLimits[i].m_maxMotorForce = btFabs(force);
		}
	}
}

void btGeneric6DofSpringConstraint::getInfo2(btConstraintInfo2* info)
{
	
	
	internalUpdateSprings(info);
	
	btGeneric6DofConstraint::getInfo2(info);
}

void btGeneric6DofSpringConstraint::setAxis(const btVector3& axis1, const btVector3& axis2)
{
	btVector3 zAxis = axis1.normalized();
	btVector3 yAxis = axis2.normalized();
	btVector3 xAxis = yAxis.cross(zAxis);  

	btTransform frameInW;
	frameInW.setIdentity();
	frameInW.getBasis().setValue(xAxis[0], yAxis[0], zAxis[0],
								 xAxis[1], yAxis[1], zAxis[1],
								 xAxis[2], yAxis[2], zAxis[2]);

	
	m_frameInA = m_rbA.getCenterOfMassTransform().inverse() * frameInW;
	m_frameInB = m_rbB.getCenterOfMassTransform().inverse() * frameInW;

	calculateTransforms();
}
