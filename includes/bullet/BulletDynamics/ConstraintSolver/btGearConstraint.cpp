



#include "btGearConstraint.h"

btGearConstraint::btGearConstraint(btRigidBody& rbA, btRigidBody& rbB, const btVector3& axisInA, const btVector3& axisInB, btScalar ratio)
	: btTypedConstraint(GEAR_CONSTRAINT_TYPE, rbA, rbB),
	  m_axisInA(axisInA),
	  m_axisInB(axisInB),
	  m_ratio(ratio)
{
}

btGearConstraint::~btGearConstraint()
{
}

void btGearConstraint::getInfo1(btConstraintInfo1* info)
{
	info->m_numConstraintRows = 1;
	info->nub = 1;
}

void btGearConstraint::getInfo2(btConstraintInfo2* info)
{
	btVector3 globalAxisA, globalAxisB;

	globalAxisA = m_rbA.getWorldTransform().getBasis() * this->m_axisInA;
	globalAxisB = m_rbB.getWorldTransform().getBasis() * this->m_axisInB;

	info->m_J1angularAxis[0] = globalAxisA[0];
	info->m_J1angularAxis[1] = globalAxisA[1];
	info->m_J1angularAxis[2] = globalAxisA[2];

	info->m_J2angularAxis[0] = m_ratio * globalAxisB[0];
	info->m_J2angularAxis[1] = m_ratio * globalAxisB[1];
	info->m_J2angularAxis[2] = m_ratio * globalAxisB[2];
}
