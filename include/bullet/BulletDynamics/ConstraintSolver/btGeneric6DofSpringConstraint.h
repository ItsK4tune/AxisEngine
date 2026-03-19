

#ifndef BT_GENERIC_6DOF_SPRING_CONSTRAINT_H
#define BT_GENERIC_6DOF_SPRING_CONSTRAINT_H

#include "LinearMath/btVector3.h"
#include "btTypedConstraint.h"
#include "btGeneric6DofConstraint.h"

#ifdef BT_USE_DOUBLE_PRECISION
#define btGeneric6DofSpringConstraintData2 btGeneric6DofSpringConstraintDoubleData2
#define btGeneric6DofSpringConstraintDataName "btGeneric6DofSpringConstraintDoubleData2"
#else
#define btGeneric6DofSpringConstraintData2 btGeneric6DofSpringConstraintData
#define btGeneric6DofSpringConstraintDataName "btGeneric6DofSpringConstraintData"
#endif  











ATTRIBUTE_ALIGNED16(class)
btGeneric6DofSpringConstraint : public btGeneric6DofConstraint
{
protected:
	bool m_springEnabled[6];
	btScalar m_equilibriumPoint[6];
	btScalar m_springStiffness[6];
	btScalar m_springDamping[6];  
	void init();
	void internalUpdateSprings(btConstraintInfo2 * info);

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btGeneric6DofSpringConstraint(btRigidBody & rbA, btRigidBody & rbB, const btTransform& frameInA, const btTransform& frameInB, bool useLinearReferenceFrameA);
	btGeneric6DofSpringConstraint(btRigidBody & rbB, const btTransform& frameInB, bool useLinearReferenceFrameB);
	void enableSpring(int index, bool onOff);
	void setStiffness(int index, btScalar stiffness);
	void setDamping(int index, btScalar damping);
	void setEquilibriumPoint();           
	void setEquilibriumPoint(int index);  
	void setEquilibriumPoint(int index, btScalar val);

	bool isSpringEnabled(int index) const
	{
		return m_springEnabled[index];
	}

	btScalar getStiffness(int index) const
	{
		return m_springStiffness[index];
	}

	btScalar getDamping(int index) const
	{
		return m_springDamping[index];
	}

	btScalar getEquilibriumPoint(int index) const
	{
		return m_equilibriumPoint[index];
	}

	virtual void setAxis(const btVector3& axis1, const btVector3& axis2);

	virtual void getInfo2(btConstraintInfo2 * info);

	virtual int calculateSerializeBufferSize() const;
	
	virtual const char* serialize(void* dataBuffer, btSerializer* serializer) const;
};

struct btGeneric6DofSpringConstraintData
{
	btGeneric6DofConstraintData m_6dofData;

	int m_springEnabled[6];
	float m_equilibriumPoint[6];
	float m_springStiffness[6];
	float m_springDamping[6];
};

struct btGeneric6DofSpringConstraintDoubleData2
{
	btGeneric6DofConstraintDoubleData2 m_6dofData;

	int m_springEnabled[6];
	double m_equilibriumPoint[6];
	double m_springStiffness[6];
	double m_springDamping[6];
};

SIMD_FORCE_INLINE int btGeneric6DofSpringConstraint::calculateSerializeBufferSize() const
{
	return sizeof(btGeneric6DofSpringConstraintData2);
}


SIMD_FORCE_INLINE const char* btGeneric6DofSpringConstraint::serialize(void* dataBuffer, btSerializer* serializer) const
{
	btGeneric6DofSpringConstraintData2* dof = (btGeneric6DofSpringConstraintData2*)dataBuffer;
	btGeneric6DofConstraint::serialize(&dof->m_6dofData, serializer);

	int i;
	for (i = 0; i < 6; i++)
	{
		dof->m_equilibriumPoint[i] = m_equilibriumPoint[i];
		dof->m_springDamping[i] = m_springDamping[i];
		dof->m_springEnabled[i] = m_springEnabled[i] ? 1 : 0;
		dof->m_springStiffness[i] = m_springStiffness[i];
	}
	return btGeneric6DofSpringConstraintDataName;
}

#endif  
