

#ifndef BT_JACOBIAN_ENTRY_H
#define BT_JACOBIAN_ENTRY_H

#include "LinearMath/btMatrix3x3.h"









ATTRIBUTE_ALIGNED16(class)
btJacobianEntry
{
public:
	btJacobianEntry(){};
	
	btJacobianEntry(
		const btMatrix3x3& world2A,
		const btMatrix3x3& world2B,
		const btVector3& rel_pos1, const btVector3& rel_pos2,
		const btVector3& jointAxis,
		const btVector3& inertiaInvA,
		const btScalar massInvA,
		const btVector3& inertiaInvB,
		const btScalar massInvB)
		: m_linearJointAxis(jointAxis)
	{
		m_aJ = world2A * (rel_pos1.cross(m_linearJointAxis));
		m_bJ = world2B * (rel_pos2.cross(-m_linearJointAxis));
		m_0MinvJt = inertiaInvA * m_aJ;
		m_1MinvJt = inertiaInvB * m_bJ;
		m_Adiag = massInvA + m_0MinvJt.dot(m_aJ) + massInvB + m_1MinvJt.dot(m_bJ);

		btAssert(m_Adiag > btScalar(0.0));
	}

	
	btJacobianEntry(const btVector3& jointAxis,
					const btMatrix3x3& world2A,
					const btMatrix3x3& world2B,
					const btVector3& inertiaInvA,
					const btVector3& inertiaInvB)
		: m_linearJointAxis(btVector3(btScalar(0.), btScalar(0.), btScalar(0.)))
	{
		m_aJ = world2A * jointAxis;
		m_bJ = world2B * -jointAxis;
		m_0MinvJt = inertiaInvA * m_aJ;
		m_1MinvJt = inertiaInvB * m_bJ;
		m_Adiag = m_0MinvJt.dot(m_aJ) + m_1MinvJt.dot(m_bJ);

		btAssert(m_Adiag > btScalar(0.0));
	}

	
	btJacobianEntry(const btVector3& axisInA,
					const btVector3& axisInB,
					const btVector3& inertiaInvA,
					const btVector3& inertiaInvB)
		: m_linearJointAxis(btVector3(btScalar(0.), btScalar(0.), btScalar(0.))), m_aJ(axisInA), m_bJ(-axisInB)
	{
		m_0MinvJt = inertiaInvA * m_aJ;
		m_1MinvJt = inertiaInvB * m_bJ;
		m_Adiag = m_0MinvJt.dot(m_aJ) + m_1MinvJt.dot(m_bJ);

		btAssert(m_Adiag > btScalar(0.0));
	}

	
	btJacobianEntry(
		const btMatrix3x3& world2A,
		const btVector3& rel_pos1, const btVector3& rel_pos2,
		const btVector3& jointAxis,
		const btVector3& inertiaInvA,
		const btScalar massInvA)
		: m_linearJointAxis(jointAxis)
	{
		m_aJ = world2A * (rel_pos1.cross(jointAxis));
		m_bJ = world2A * (rel_pos2.cross(-jointAxis));
		m_0MinvJt = inertiaInvA * m_aJ;
		m_1MinvJt = btVector3(btScalar(0.), btScalar(0.), btScalar(0.));
		m_Adiag = massInvA + m_0MinvJt.dot(m_aJ);

		btAssert(m_Adiag > btScalar(0.0));
	}

	btScalar getDiagonal() const { return m_Adiag; }

	
	btScalar getNonDiagonal(const btJacobianEntry& jacB, const btScalar massInvA) const
	{
		const btJacobianEntry& jacA = *this;
		btScalar lin = massInvA * jacA.m_linearJointAxis.dot(jacB.m_linearJointAxis);
		btScalar ang = jacA.m_0MinvJt.dot(jacB.m_aJ);
		return lin + ang;
	}

	
	btScalar getNonDiagonal(const btJacobianEntry& jacB, const btScalar massInvA, const btScalar massInvB) const
	{
		const btJacobianEntry& jacA = *this;
		btVector3 lin = jacA.m_linearJointAxis * jacB.m_linearJointAxis;
		btVector3 ang0 = jacA.m_0MinvJt * jacB.m_aJ;
		btVector3 ang1 = jacA.m_1MinvJt * jacB.m_bJ;
		btVector3 lin0 = massInvA * lin;
		btVector3 lin1 = massInvB * lin;
		btVector3 sum = ang0 + ang1 + lin0 + lin1;
		return sum[0] + sum[1] + sum[2];
	}

	btScalar getRelativeVelocity(const btVector3& linvelA, const btVector3& angvelA, const btVector3& linvelB, const btVector3& angvelB)
	{
		btVector3 linrel = linvelA - linvelB;
		btVector3 angvela = angvelA * m_aJ;
		btVector3 angvelb = angvelB * m_bJ;
		linrel *= m_linearJointAxis;
		angvela += angvelb;
		angvela += linrel;
		btScalar rel_vel2 = angvela[0] + angvela[1] + angvela[2];
		return rel_vel2 + SIMD_EPSILON;
	}
	

	btVector3 m_linearJointAxis;
	btVector3 m_aJ;
	btVector3 m_bJ;
	btVector3 m_0MinvJt;
	btVector3 m_1MinvJt;
	
	btScalar m_Adiag;
};

#endif  
