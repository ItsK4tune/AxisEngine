






#ifndef BT_GENERIC_6DOF_CONSTRAINT_H
#define BT_GENERIC_6DOF_CONSTRAINT_H

#include "LinearMath/btVector3.h"
#include "btJacobianEntry.h"
#include "btTypedConstraint.h"

class btRigidBody;

#ifdef BT_USE_DOUBLE_PRECISION
#define btGeneric6DofConstraintData2 btGeneric6DofConstraintDoubleData2
#define btGeneric6DofConstraintDataName "btGeneric6DofConstraintDoubleData2"
#else
#define btGeneric6DofConstraintData2 btGeneric6DofConstraintData
#define btGeneric6DofConstraintDataName "btGeneric6DofConstraintData"
#endif  


class btRotationalLimitMotor
{
public:
	
	
	btScalar m_loLimit;         
	btScalar m_hiLimit;         
	btScalar m_targetVelocity;  
	btScalar m_maxMotorForce;   
	btScalar m_maxLimitForce;   
	btScalar m_damping;         
	btScalar m_limitSoftness;   
	btScalar m_normalCFM;       
	btScalar m_stopERP;         
	btScalar m_stopCFM;         
	btScalar m_bounce;          
	bool m_enableMotor;

	

	
	
	btScalar m_currentLimitError;  
	btScalar m_currentPosition;    
	int m_currentLimit;            
	btScalar m_accumulatedImpulse;
	

	btRotationalLimitMotor()
	{
		m_accumulatedImpulse = 0.f;
		m_targetVelocity = 0;
		m_maxMotorForce = 6.0f;
		m_maxLimitForce = 300.0f;
		m_loLimit = 1.0f;
		m_hiLimit = -1.0f;
		m_normalCFM = 0.f;
		m_stopERP = 0.2f;
		m_stopCFM = 0.f;
		m_bounce = 0.0f;
		m_damping = 1.0f;
		m_limitSoftness = 0.5f;
		m_currentLimit = 0;
		m_currentLimitError = 0;
		m_enableMotor = false;
	}

	btRotationalLimitMotor(const btRotationalLimitMotor& limot)
	{
		m_targetVelocity = limot.m_targetVelocity;
		m_maxMotorForce = limot.m_maxMotorForce;
		m_limitSoftness = limot.m_limitSoftness;
		m_loLimit = limot.m_loLimit;
		m_hiLimit = limot.m_hiLimit;
		m_normalCFM = limot.m_normalCFM;
		m_stopERP = limot.m_stopERP;
		m_stopCFM = limot.m_stopCFM;
		m_bounce = limot.m_bounce;
		m_currentLimit = limot.m_currentLimit;
		m_currentLimitError = limot.m_currentLimitError;
		m_enableMotor = limot.m_enableMotor;
	}

	
	bool isLimited() const
	{
		if (m_loLimit > m_hiLimit) return false;
		return true;
	}

	
	bool needApplyTorques() const
	{
		if (m_currentLimit == 0 && m_enableMotor == false) return false;
		return true;
	}

	
	
	int testLimitValue(btScalar test_value);

	
	btScalar solveAngularLimits(btScalar timeStep, btVector3& axis, btScalar jacDiagABInv, btRigidBody* body0, btRigidBody* body1);
};

class btTranslationalLimitMotor
{
public:
	btVector3 m_lowerLimit;  
	btVector3 m_upperLimit;  
	btVector3 m_accumulatedImpulse;
	
	
	btScalar m_limitSoftness;  
	btScalar m_damping;        
	btScalar m_restitution;    
	btVector3 m_normalCFM;     
	btVector3 m_stopERP;       
	btVector3 m_stopCFM;       
							   
	bool m_enableMotor[3];
	btVector3 m_targetVelocity;     
	btVector3 m_maxMotorForce;      
	btVector3 m_currentLimitError;  
	btVector3 m_currentLinearDiff;  
	int m_currentLimit[3];          

	btTranslationalLimitMotor()
	{
		m_lowerLimit.setValue(0.f, 0.f, 0.f);
		m_upperLimit.setValue(0.f, 0.f, 0.f);
		m_accumulatedImpulse.setValue(0.f, 0.f, 0.f);
		m_normalCFM.setValue(0.f, 0.f, 0.f);
		m_stopERP.setValue(0.2f, 0.2f, 0.2f);
		m_stopCFM.setValue(0.f, 0.f, 0.f);

		m_limitSoftness = 0.7f;
		m_damping = btScalar(1.0f);
		m_restitution = btScalar(0.5f);
		for (int i = 0; i < 3; i++)
		{
			m_enableMotor[i] = false;
			m_targetVelocity[i] = btScalar(0.f);
			m_maxMotorForce[i] = btScalar(0.f);
		}
	}

	btTranslationalLimitMotor(const btTranslationalLimitMotor& other)
	{
		m_lowerLimit = other.m_lowerLimit;
		m_upperLimit = other.m_upperLimit;
		m_accumulatedImpulse = other.m_accumulatedImpulse;

		m_limitSoftness = other.m_limitSoftness;
		m_damping = other.m_damping;
		m_restitution = other.m_restitution;
		m_normalCFM = other.m_normalCFM;
		m_stopERP = other.m_stopERP;
		m_stopCFM = other.m_stopCFM;

		for (int i = 0; i < 3; i++)
		{
			m_enableMotor[i] = other.m_enableMotor[i];
			m_targetVelocity[i] = other.m_targetVelocity[i];
			m_maxMotorForce[i] = other.m_maxMotorForce[i];
		}
	}

	
	
	inline bool isLimited(int limitIndex) const
	{
		return (m_upperLimit[limitIndex] >= m_lowerLimit[limitIndex]);
	}
	inline bool needApplyForce(int limitIndex) const
	{
		if (m_currentLimit[limitIndex] == 0 && m_enableMotor[limitIndex] == false) return false;
		return true;
	}
	int testLimitValue(int limitIndex, btScalar test_value);

	btScalar solveLinearAxis(
		btScalar timeStep,
		btScalar jacDiagABInv,
		btRigidBody& body1, const btVector3& pointInA,
		btRigidBody& body2, const btVector3& pointInB,
		int limit_index,
		const btVector3& axis_normal_on_a,
		const btVector3& anchorPos);
};

enum bt6DofFlags
{
	BT_6DOF_FLAGS_CFM_NORM = 1,
	BT_6DOF_FLAGS_CFM_STOP = 2,
	BT_6DOF_FLAGS_ERP_STOP = 4
};
#define BT_6DOF_FLAGS_AXIS_SHIFT 3  



ATTRIBUTE_ALIGNED16(class)
btGeneric6DofConstraint : public btTypedConstraint
{
protected:
	
	
	btTransform m_frameInA;  
	btTransform m_frameInB;  
	

	
	
	btJacobianEntry m_jacLinear[3];  
	btJacobianEntry m_jacAng[3];     
	

	
	
	btTranslationalLimitMotor m_linearLimits;
	

	
	
	btRotationalLimitMotor m_angularLimits[3];
	

protected:
	
	
	btScalar m_timeStep;
	btTransform m_calculatedTransformA;
	btTransform m_calculatedTransformB;
	btVector3 m_calculatedAxisAngleDiff;
	btVector3 m_calculatedAxis[3];
	btVector3 m_calculatedLinearDiff;
	btScalar m_factA;
	btScalar m_factB;
	bool m_hasStaticBody;

	btVector3 m_AnchorPos;  

	bool m_useLinearReferenceFrameA;
	bool m_useOffsetForConstraintFrame;

	int m_flags;

	

	btGeneric6DofConstraint& operator=(btGeneric6DofConstraint& other)
	{
		btAssert(0);
		(void)other;
		return *this;
	}

	int setAngularLimits(btConstraintInfo2 * info, int row_offset, const btTransform& transA, const btTransform& transB, const btVector3& linVelA, const btVector3& linVelB, const btVector3& angVelA, const btVector3& angVelB);

	int setLinearLimits(btConstraintInfo2 * info, int row, const btTransform& transA, const btTransform& transB, const btVector3& linVelA, const btVector3& linVelB, const btVector3& angVelA, const btVector3& angVelB);

	void buildLinearJacobian(
		btJacobianEntry & jacLinear, const btVector3& normalWorld,
		const btVector3& pivotAInW, const btVector3& pivotBInW);

	void buildAngularJacobian(btJacobianEntry & jacAngular, const btVector3& jointAxisW);

	
	void calculateLinearInfo();

	
	void calculateAngleInfo();

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	
	bool m_useSolveConstraintObsolete;

	btGeneric6DofConstraint(btRigidBody & rbA, btRigidBody & rbB, const btTransform& frameInA, const btTransform& frameInB, bool useLinearReferenceFrameA);
	btGeneric6DofConstraint(btRigidBody & rbB, const btTransform& frameInB, bool useLinearReferenceFrameB);

	
	
	void calculateTransforms(const btTransform& transA, const btTransform& transB);

	void calculateTransforms();

	
	
	const btTransform& getCalculatedTransformA() const
	{
		return m_calculatedTransformA;
	}

	
	
	const btTransform& getCalculatedTransformB() const
	{
		return m_calculatedTransformB;
	}

	const btTransform& getFrameOffsetA() const
	{
		return m_frameInA;
	}

	const btTransform& getFrameOffsetB() const
	{
		return m_frameInB;
	}

	btTransform& getFrameOffsetA()
	{
		return m_frameInA;
	}

	btTransform& getFrameOffsetB()
	{
		return m_frameInB;
	}

	
	virtual void buildJacobian();

	virtual void getInfo1(btConstraintInfo1 * info);

	void getInfo1NonVirtual(btConstraintInfo1 * info);

	virtual void getInfo2(btConstraintInfo2 * info);

	void getInfo2NonVirtual(btConstraintInfo2 * info, const btTransform& transA, const btTransform& transB, const btVector3& linVelA, const btVector3& linVelB, const btVector3& angVelA, const btVector3& angVelB);

	void updateRHS(btScalar timeStep);

	
	
	btVector3 getAxis(int axis_index) const;

	
	
	btScalar getAngle(int axis_index) const;

	
	
	btScalar getRelativePivotPosition(int axis_index) const;

	void setFrames(const btTransform& frameA, const btTransform& frameB);

	
	
	bool testAngularLimitMotor(int axis_index);

	void setLinearLowerLimit(const btVector3& linearLower)
	{
		m_linearLimits.m_lowerLimit = linearLower;
	}

	void getLinearLowerLimit(btVector3 & linearLower) const
	{
		linearLower = m_linearLimits.m_lowerLimit;
	}

	void setLinearUpperLimit(const btVector3& linearUpper)
	{
		m_linearLimits.m_upperLimit = linearUpper;
	}

	void getLinearUpperLimit(btVector3 & linearUpper) const
	{
		linearUpper = m_linearLimits.m_upperLimit;
	}

	void setAngularLowerLimit(const btVector3& angularLower)
	{
		for (int i = 0; i < 3; i++)
			m_angularLimits[i].m_loLimit = btNormalizeAngle(angularLower[i]);
	}

	void getAngularLowerLimit(btVector3 & angularLower) const
	{
		for (int i = 0; i < 3; i++)
			angularLower[i] = m_angularLimits[i].m_loLimit;
	}

	void setAngularUpperLimit(const btVector3& angularUpper)
	{
		for (int i = 0; i < 3; i++)
			m_angularLimits[i].m_hiLimit = btNormalizeAngle(angularUpper[i]);
	}

	void getAngularUpperLimit(btVector3 & angularUpper) const
	{
		for (int i = 0; i < 3; i++)
			angularUpper[i] = m_angularLimits[i].m_hiLimit;
	}

	
	btRotationalLimitMotor* getRotationalLimitMotor(int index)
	{
		return &m_angularLimits[index];
	}

	
	btTranslationalLimitMotor* getTranslationalLimitMotor()
	{
		return &m_linearLimits;
	}

	
	void setLimit(int axis, btScalar lo, btScalar hi)
	{
		if (axis < 3)
		{
			m_linearLimits.m_lowerLimit[axis] = lo;
			m_linearLimits.m_upperLimit[axis] = hi;
		}
		else
		{
			lo = btNormalizeAngle(lo);
			hi = btNormalizeAngle(hi);
			m_angularLimits[axis - 3].m_loLimit = lo;
			m_angularLimits[axis - 3].m_hiLimit = hi;
		}
	}

	
	
	bool isLimited(int limitIndex) const
	{
		if (limitIndex < 3)
		{
			return m_linearLimits.isLimited(limitIndex);
		}
		return m_angularLimits[limitIndex - 3].isLimited();
	}

	virtual void calcAnchorPos(void);  

	int get_limit_motor_info2(btRotationalLimitMotor * limot,
							  const btTransform& transA, const btTransform& transB, const btVector3& linVelA, const btVector3& linVelB, const btVector3& angVelA, const btVector3& angVelB,
							  btConstraintInfo2* info, int row, btVector3& ax1, int rotational, int rotAllowed = false);

	
	bool getUseFrameOffset() const { return m_useOffsetForConstraintFrame; }
	void setUseFrameOffset(bool frameOffsetOnOff) { m_useOffsetForConstraintFrame = frameOffsetOnOff; }

	bool getUseLinearReferenceFrameA() const { return m_useLinearReferenceFrameA; }
	void setUseLinearReferenceFrameA(bool linearReferenceFrameA) { m_useLinearReferenceFrameA = linearReferenceFrameA; }

	
	
	virtual void setParam(int num, btScalar value, int axis = -1);
	
	virtual btScalar getParam(int num, int axis = -1) const;

	void setAxis(const btVector3& axis1, const btVector3& axis2);

	virtual int getFlags() const
	{
		return m_flags;
	}

	virtual int calculateSerializeBufferSize() const;

	
	virtual const char* serialize(void* dataBuffer, btSerializer* serializer) const;
};

struct btGeneric6DofConstraintData
{
	btTypedConstraintData m_typeConstraintData;
	btTransformFloatData m_rbAFrame;  
	btTransformFloatData m_rbBFrame;

	btVector3FloatData m_linearUpperLimit;
	btVector3FloatData m_linearLowerLimit;

	btVector3FloatData m_angularUpperLimit;
	btVector3FloatData m_angularLowerLimit;

	int m_useLinearReferenceFrameA;
	int m_useOffsetForConstraintFrame;
};

struct btGeneric6DofConstraintDoubleData2
{
	btTypedConstraintDoubleData m_typeConstraintData;
	btTransformDoubleData m_rbAFrame;  
	btTransformDoubleData m_rbBFrame;

	btVector3DoubleData m_linearUpperLimit;
	btVector3DoubleData m_linearLowerLimit;

	btVector3DoubleData m_angularUpperLimit;
	btVector3DoubleData m_angularLowerLimit;

	int m_useLinearReferenceFrameA;
	int m_useOffsetForConstraintFrame;
};

SIMD_FORCE_INLINE int btGeneric6DofConstraint::calculateSerializeBufferSize() const
{
	return sizeof(btGeneric6DofConstraintData2);
}


SIMD_FORCE_INLINE const char* btGeneric6DofConstraint::serialize(void* dataBuffer, btSerializer* serializer) const
{
	btGeneric6DofConstraintData2* dof = (btGeneric6DofConstraintData2*)dataBuffer;
	btTypedConstraint::serialize(&dof->m_typeConstraintData, serializer);

	m_frameInA.serialize(dof->m_rbAFrame);
	m_frameInB.serialize(dof->m_rbBFrame);

	int i;
	for (i = 0; i < 3; i++)
	{
		dof->m_angularLowerLimit.m_floats[i] = m_angularLimits[i].m_loLimit;
		dof->m_angularUpperLimit.m_floats[i] = m_angularLimits[i].m_hiLimit;
		dof->m_linearLowerLimit.m_floats[i] = m_linearLimits.m_lowerLimit[i];
		dof->m_linearUpperLimit.m_floats[i] = m_linearLimits.m_upperLimit[i];
	}

	dof->m_useLinearReferenceFrameA = m_useLinearReferenceFrameA ? 1 : 0;
	dof->m_useOffsetForConstraintFrame = m_useOffsetForConstraintFrame ? 1 : 0;

	return btGeneric6DofConstraintDataName;
}

#endif  
