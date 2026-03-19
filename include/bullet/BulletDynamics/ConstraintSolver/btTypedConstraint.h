

#ifndef BT_TYPED_CONSTRAINT_H
#define BT_TYPED_CONSTRAINT_H

#include "LinearMath/btScalar.h"
#include "btSolverConstraint.h"
#include "BulletDynamics/Dynamics/btRigidBody.h"

#ifdef BT_USE_DOUBLE_PRECISION
#define btTypedConstraintData2 btTypedConstraintDoubleData
#define btTypedConstraintDataName "btTypedConstraintDoubleData"
#else
#define btTypedConstraintData2 btTypedConstraintFloatData
#define btTypedConstraintDataName "btTypedConstraintFloatData"
#endif  

class btSerializer;


enum btTypedConstraintType
{
	POINT2POINT_CONSTRAINT_TYPE = 3,
	HINGE_CONSTRAINT_TYPE,
	CONETWIST_CONSTRAINT_TYPE,
	D6_CONSTRAINT_TYPE,
	SLIDER_CONSTRAINT_TYPE,
	CONTACT_CONSTRAINT_TYPE,
	D6_SPRING_CONSTRAINT_TYPE,
	GEAR_CONSTRAINT_TYPE,
	FIXED_CONSTRAINT_TYPE,
	D6_SPRING_2_CONSTRAINT_TYPE,
	MAX_CONSTRAINT_TYPE
};

enum btConstraintParams
{
	BT_CONSTRAINT_ERP = 1,
	BT_CONSTRAINT_STOP_ERP,
	BT_CONSTRAINT_CFM,
	BT_CONSTRAINT_STOP_CFM
};

#if 1
#define btAssertConstrParams(_par) btAssert(_par)
#else
#define btAssertConstrParams(_par)
#endif

ATTRIBUTE_ALIGNED16(struct)
btJointFeedback
{
	BT_DECLARE_ALIGNED_ALLOCATOR();
	btVector3 m_appliedForceBodyA;
	btVector3 m_appliedTorqueBodyA;
	btVector3 m_appliedForceBodyB;
	btVector3 m_appliedTorqueBodyB;
};


ATTRIBUTE_ALIGNED16(class)
btTypedConstraint : public btTypedObject
{
	int m_userConstraintType;

	union {
		int m_userConstraintId;
		void* m_userConstraintPtr;
	};

	btScalar m_breakingImpulseThreshold;
	bool m_isEnabled;
	bool m_needsFeedback;
	int m_overrideNumSolverIterations;

	btTypedConstraint& operator=(btTypedConstraint& other)
	{
		btAssert(0);
		(void)other;
		return *this;
	}

protected:
	btRigidBody& m_rbA;
	btRigidBody& m_rbB;
	btScalar m_appliedImpulse;
	btScalar m_dbgDrawSize;
	btJointFeedback* m_jointFeedback;

	
	btScalar getMotorFactor(btScalar pos, btScalar lowLim, btScalar uppLim, btScalar vel, btScalar timeFact);

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	virtual ~btTypedConstraint(){};
	btTypedConstraint(btTypedConstraintType type, btRigidBody & rbA);
	btTypedConstraint(btTypedConstraintType type, btRigidBody & rbA, btRigidBody & rbB);

	struct btConstraintInfo1
	{
		int m_numConstraintRows, nub;
	};

	static btRigidBody& getFixedBody();

	struct btConstraintInfo2
	{
		
		
		btScalar fps, erp;

		
		
		
		
		btScalar *m_J1linearAxis, *m_J1angularAxis, *m_J2linearAxis, *m_J2angularAxis;

		
		int rowskip;

		
		
		
		btScalar *m_constraintError, *cfm;

		
		btScalar *m_lowerLimit, *m_upperLimit;

		
		int m_numIterations;

		
		btScalar m_damping;
	};

	int getOverrideNumSolverIterations() const
	{
		return m_overrideNumSolverIterations;
	}

	
	
	void setOverrideNumSolverIterations(int overideNumIterations)
	{
		m_overrideNumSolverIterations = overideNumIterations;
	}

	
	virtual void buildJacobian(){};

	
	virtual void setupSolverConstraint(btConstraintArray & ca, int solverBodyA, int solverBodyB, btScalar timeStep)
	{
		(void)ca;
		(void)solverBodyA;
		(void)solverBodyB;
		(void)timeStep;
	}

	
	virtual void getInfo1(btConstraintInfo1 * info) = 0;

	
	virtual void getInfo2(btConstraintInfo2 * info) = 0;

	
	void internalSetAppliedImpulse(btScalar appliedImpulse)
	{
		m_appliedImpulse = appliedImpulse;
	}
	
	btScalar internalGetAppliedImpulse()
	{
		return m_appliedImpulse;
	}

	btScalar getBreakingImpulseThreshold() const
	{
		return m_breakingImpulseThreshold;
	}

	void setBreakingImpulseThreshold(btScalar threshold)
	{
		m_breakingImpulseThreshold = threshold;
	}

	bool isEnabled() const
	{
		return m_isEnabled;
	}

	void setEnabled(bool enabled)
	{
		m_isEnabled = enabled;
	}

	
	virtual void solveConstraintObsolete(btSolverBody& , btSolverBody& , btScalar ){};

	const btRigidBody& getRigidBodyA() const
	{
		return m_rbA;
	}
	const btRigidBody& getRigidBodyB() const
	{
		return m_rbB;
	}

	btRigidBody& getRigidBodyA()
	{
		return m_rbA;
	}
	btRigidBody& getRigidBodyB()
	{
		return m_rbB;
	}

	int getUserConstraintType() const
	{
		return m_userConstraintType;
	}

	void setUserConstraintType(int userConstraintType)
	{
		m_userConstraintType = userConstraintType;
	};

	void setUserConstraintId(int uid)
	{
		m_userConstraintId = uid;
	}

	int getUserConstraintId() const
	{
		return m_userConstraintId;
	}

	void setUserConstraintPtr(void* ptr)
	{
		m_userConstraintPtr = ptr;
	}

	void* getUserConstraintPtr()
	{
		return m_userConstraintPtr;
	}

	void setJointFeedback(btJointFeedback * jointFeedback)
	{
		m_jointFeedback = jointFeedback;
	}

	const btJointFeedback* getJointFeedback() const
	{
		return m_jointFeedback;
	}

	btJointFeedback* getJointFeedback()
	{
		return m_jointFeedback;
	}

	int getUid() const
	{
		return m_userConstraintId;
	}

	bool needsFeedback() const
	{
		return m_needsFeedback;
	}

	
	
	void enableFeedback(bool needsFeedback)
	{
		m_needsFeedback = needsFeedback;
	}

	
	
	btScalar getAppliedImpulse() const
	{
		btAssert(m_needsFeedback);
		return m_appliedImpulse;
	}

	btTypedConstraintType getConstraintType() const
	{
		return btTypedConstraintType(m_objectType);
	}

	void setDbgDrawSize(btScalar dbgDrawSize)
	{
		m_dbgDrawSize = dbgDrawSize;
	}
	btScalar getDbgDrawSize()
	{
		return m_dbgDrawSize;
	}

	
	
	virtual void setParam(int num, btScalar value, int axis = -1) = 0;

	
	virtual btScalar getParam(int num, int axis = -1) const = 0;

	virtual int calculateSerializeBufferSize() const;

	
	virtual const char* serialize(void* dataBuffer, btSerializer* serializer) const;
};



SIMD_FORCE_INLINE btScalar btAdjustAngleToLimits(btScalar angleInRadians, btScalar angleLowerLimitInRadians, btScalar angleUpperLimitInRadians)
{
	if (angleLowerLimitInRadians >= angleUpperLimitInRadians)
	{
		return angleInRadians;
	}
	else if (angleInRadians < angleLowerLimitInRadians)
	{
		btScalar diffLo = btFabs(btNormalizeAngle(angleLowerLimitInRadians - angleInRadians));
		btScalar diffHi = btFabs(btNormalizeAngle(angleUpperLimitInRadians - angleInRadians));
		return (diffLo < diffHi) ? angleInRadians : (angleInRadians + SIMD_2_PI);
	}
	else if (angleInRadians > angleUpperLimitInRadians)
	{
		btScalar diffHi = btFabs(btNormalizeAngle(angleInRadians - angleUpperLimitInRadians));
		btScalar diffLo = btFabs(btNormalizeAngle(angleInRadians - angleLowerLimitInRadians));
		return (diffLo < diffHi) ? (angleInRadians - SIMD_2_PI) : angleInRadians;
	}
	else
	{
		return angleInRadians;
	}
}




struct	btTypedConstraintFloatData
{
	btRigidBodyFloatData		*m_rbA;
	btRigidBodyFloatData		*m_rbB;
	char	*m_name;

	int	m_objectType;
	int	m_userConstraintType;
	int	m_userConstraintId;
	int	m_needsFeedback;

	float	m_appliedImpulse;
	float	m_dbgDrawSize;

	int	m_disableCollisionsBetweenLinkedBodies;
	int	m_overrideNumSolverIterations;

	float	m_breakingImpulseThreshold;
	int		m_isEnabled;
	
};





#define BT_BACKWARDS_COMPATIBLE_SERIALIZATION
#ifdef BT_BACKWARDS_COMPATIBLE_SERIALIZATION

struct	btTypedConstraintData
{
	btRigidBodyData		*m_rbA;
	btRigidBodyData		*m_rbB;
	char	*m_name;

	int	m_objectType;
	int	m_userConstraintType;
	int	m_userConstraintId;
	int	m_needsFeedback;

	float	m_appliedImpulse;
	float	m_dbgDrawSize;

	int	m_disableCollisionsBetweenLinkedBodies;
	int	m_overrideNumSolverIterations;

	float	m_breakingImpulseThreshold;
	int		m_isEnabled;
	
};
#endif 

struct	btTypedConstraintDoubleData
{
	btRigidBodyDoubleData		*m_rbA;
	btRigidBodyDoubleData		*m_rbB;
	char	*m_name;

	int	m_objectType;
	int	m_userConstraintType;
	int	m_userConstraintId;
	int	m_needsFeedback;

	double	m_appliedImpulse;
	double	m_dbgDrawSize;

	int	m_disableCollisionsBetweenLinkedBodies;
	int	m_overrideNumSolverIterations;

	double	m_breakingImpulseThreshold;
	int		m_isEnabled;
	char	padding[4];
	
};



SIMD_FORCE_INLINE int btTypedConstraint::calculateSerializeBufferSize() const
{
	return sizeof(btTypedConstraintData2);
}

class btAngularLimit
{
private:
	btScalar
		m_center,
		m_halfRange,
		m_softness,
		m_biasFactor,
		m_relaxationFactor,
		m_correction,
		m_sign;

	bool
		m_solveLimit;

public:
	
	btAngularLimit()
		: m_center(0.0f),
		  m_halfRange(-1.0f),
		  m_softness(0.9f),
		  m_biasFactor(0.3f),
		  m_relaxationFactor(1.0f),
		  m_correction(0.0f),
		  m_sign(0.0f),
		  m_solveLimit(false)
	{
	}

	
	
	
	void set(btScalar low, btScalar high, btScalar _softness = 0.9f, btScalar _biasFactor = 0.3f, btScalar _relaxationFactor = 1.0f);

	
	
	void test(const btScalar angle);

	
	inline btScalar getSoftness() const
	{
		return m_softness;
	}

	
	inline btScalar getBiasFactor() const
	{
		return m_biasFactor;
	}

	
	inline btScalar getRelaxationFactor() const
	{
		return m_relaxationFactor;
	}

	
	inline btScalar getCorrection() const
	{
		return m_correction;
	}

	
	inline btScalar getSign() const
	{
		return m_sign;
	}

	
	inline btScalar getHalfRange() const
	{
		return m_halfRange;
	}

	
	inline bool isLimit() const
	{
		return m_solveLimit;
	}

	
	
	void fit(btScalar& angle) const;

	
	btScalar getError() const;

	btScalar getLow() const;

	btScalar getHigh() const;
};

#endif  
