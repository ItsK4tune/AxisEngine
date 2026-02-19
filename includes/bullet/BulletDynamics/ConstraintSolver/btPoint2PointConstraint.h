

#ifndef BT_POINT2POINTCONSTRAINT_H
#define BT_POINT2POINTCONSTRAINT_H

#include "LinearMath/btVector3.h"
#include "btJacobianEntry.h"
#include "btTypedConstraint.h"

class btRigidBody;

#ifdef BT_USE_DOUBLE_PRECISION
#define btPoint2PointConstraintData2 btPoint2PointConstraintDoubleData2
#define btPoint2PointConstraintDataName "btPoint2PointConstraintDoubleData2"
#else
#define btPoint2PointConstraintData2 btPoint2PointConstraintFloatData
#define btPoint2PointConstraintDataName "btPoint2PointConstraintFloatData"
#endif  

struct btConstraintSetting
{
	btConstraintSetting() : m_tau(btScalar(0.3)),
							m_damping(btScalar(1.)),
							m_impulseClamp(btScalar(0.))
	{
	}
	btScalar m_tau;
	btScalar m_damping;
	btScalar m_impulseClamp;
};

enum btPoint2PointFlags
{
	BT_P2P_FLAGS_ERP = 1,
	BT_P2P_FLAGS_CFM = 2
};


ATTRIBUTE_ALIGNED16(class)
btPoint2PointConstraint : public btTypedConstraint
{
#ifdef IN_PARALLELL_SOLVER
public:
#endif
	btJacobianEntry m_jac[3];  

	btVector3 m_pivotInA;
	btVector3 m_pivotInB;

	int m_flags;
	btScalar m_erp;
	btScalar m_cfm;

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	
	bool m_useSolveConstraintObsolete;

	btConstraintSetting m_setting;

	btPoint2PointConstraint(btRigidBody & rbA, btRigidBody & rbB, const btVector3& pivotInA, const btVector3& pivotInB);

	btPoint2PointConstraint(btRigidBody & rbA, const btVector3& pivotInA);

	virtual void buildJacobian();

	virtual void getInfo1(btConstraintInfo1 * info);

	void getInfo1NonVirtual(btConstraintInfo1 * info);

	virtual void getInfo2(btConstraintInfo2 * info);

	void getInfo2NonVirtual(btConstraintInfo2 * info, const btTransform& body0_trans, const btTransform& body1_trans);

	void updateRHS(btScalar timeStep);

	void setPivotA(const btVector3& pivotA)
	{
		m_pivotInA = pivotA;
	}

	void setPivotB(const btVector3& pivotB)
	{
		m_pivotInB = pivotB;
	}

	const btVector3& getPivotInA() const
	{
		return m_pivotInA;
	}

	const btVector3& getPivotInB() const
	{
		return m_pivotInB;
	}

	
	
	virtual void setParam(int num, btScalar value, int axis = -1);
	
	virtual btScalar getParam(int num, int axis = -1) const;

	virtual int getFlags() const
	{
		return m_flags;
	}

	virtual int calculateSerializeBufferSize() const;

	
	virtual const char* serialize(void* dataBuffer, btSerializer* serializer) const;
};


struct btPoint2PointConstraintFloatData
{
	btTypedConstraintData m_typeConstraintData;
	btVector3FloatData m_pivotInA;
	btVector3FloatData m_pivotInB;
};


struct btPoint2PointConstraintDoubleData2
{
	btTypedConstraintDoubleData m_typeConstraintData;
	btVector3DoubleData m_pivotInA;
	btVector3DoubleData m_pivotInB;
};

#ifdef BT_BACKWARDS_COMPATIBLE_SERIALIZATION



struct btPoint2PointConstraintDoubleData
{
	btTypedConstraintData m_typeConstraintData;
	btVector3DoubleData m_pivotInA;
	btVector3DoubleData m_pivotInB;
};
#endif  

SIMD_FORCE_INLINE int btPoint2PointConstraint::calculateSerializeBufferSize() const
{
	return sizeof(btPoint2PointConstraintData2);
}


SIMD_FORCE_INLINE const char* btPoint2PointConstraint::serialize(void* dataBuffer, btSerializer* serializer) const
{
	btPoint2PointConstraintData2* p2pData = (btPoint2PointConstraintData2*)dataBuffer;

	btTypedConstraint::serialize(&p2pData->m_typeConstraintData, serializer);
	m_pivotInA.serialize(p2pData->m_pivotInA);
	m_pivotInB.serialize(p2pData->m_pivotInB);

	return btPoint2PointConstraintDataName;
}

#endif  
