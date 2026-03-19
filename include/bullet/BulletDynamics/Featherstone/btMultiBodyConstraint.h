

#ifndef BT_MULTIBODY_CONSTRAINT_H
#define BT_MULTIBODY_CONSTRAINT_H

#include "LinearMath/btScalar.h"
#include "LinearMath/btAlignedObjectArray.h"
#include "btMultiBody.h"



enum btTypedMultiBodyConstraintType
{
	MULTIBODY_CONSTRAINT_LIMIT=3,
	MULTIBODY_CONSTRAINT_1DOF_JOINT_MOTOR,
	MULTIBODY_CONSTRAINT_GEAR,
	MULTIBODY_CONSTRAINT_POINT_TO_POINT,
	MULTIBODY_CONSTRAINT_SLIDER,
	MULTIBODY_CONSTRAINT_SPHERICAL_MOTOR,
	MULTIBODY_CONSTRAINT_FIXED,
	MULTIBODY_CONSTRAINT_SPHERICAL_LIMIT,
	MAX_MULTIBODY_CONSTRAINT_TYPE,
};

class btMultiBody;
struct btSolverInfo;

#include "btMultiBodySolverConstraint.h"

struct btMultiBodyJacobianData
{
	btAlignedObjectArray<btScalar> m_jacobians;
	btAlignedObjectArray<btScalar> m_deltaVelocitiesUnitImpulse;  
	btAlignedObjectArray<btScalar> m_deltaVelocities;             
	btAlignedObjectArray<btScalar> scratch_r;
	btAlignedObjectArray<btVector3> scratch_v;
	btAlignedObjectArray<btMatrix3x3> scratch_m;
	btAlignedObjectArray<btSolverBody>* m_solverBodyPool;
	int m_fixedBodyId;
};

ATTRIBUTE_ALIGNED16(class)
btMultiBodyConstraint
{
protected:
	btMultiBody* m_bodyA;
	btMultiBody* m_bodyB;
	int m_linkA;
	int m_linkB;

	int m_type; 

	int m_numRows;
	int m_jacSizeA;
	int m_jacSizeBoth;
	int m_posOffset;

	bool m_isUnilateral;
	int m_numDofsFinalized;
	btScalar m_maxAppliedImpulse;

	
	
	
	
	
	btAlignedObjectArray<btScalar> m_data;

	void applyDeltaVee(btMultiBodyJacobianData & data, btScalar * delta_vee, btScalar impulse, int velocityIndex, int ndof);

	btScalar fillMultiBodyConstraint(btMultiBodySolverConstraint & solverConstraint,
									 btMultiBodyJacobianData & data,
									 btScalar * jacOrgA, btScalar * jacOrgB,
									 const btVector3& constraintNormalAng,

									 const btVector3& constraintNormalLin,
									 const btVector3& posAworld, const btVector3& posBworld,
									 btScalar posError,
									 const btContactSolverInfo& infoGlobal,
									 btScalar lowerLimit, btScalar upperLimit,
									 bool angConstraint = false,

									 btScalar relaxation = 1.f,
									 bool isFriction = false, btScalar desiredVelocity = 0, btScalar cfmSlip = 0, btScalar damping = 1.0);

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btMultiBodyConstraint(btMultiBody * bodyA, btMultiBody * bodyB, int linkA, int linkB, int numRows, bool isUnilateral, int type);
	virtual ~btMultiBodyConstraint();

	void updateJacobianSizes();
	void allocateJacobiansMultiDof();

	int getConstraintType() const
	{
		return m_type;
	}
	
	virtual void setFrameInB(const btMatrix3x3& frameInB) {}
	virtual void setPivotInB(const btVector3& pivotInB) {}

	virtual void finalizeMultiDof() = 0;

	virtual int getIslandIdA() const = 0;
	virtual int getIslandIdB() const = 0;

	virtual void createConstraintRows(btMultiBodyConstraintArray & constraintRows,
									  btMultiBodyJacobianData & data,
									  const btContactSolverInfo& infoGlobal) = 0;

	int getNumRows() const
	{
		return m_numRows;
	}

	btMultiBody* getMultiBodyA()
	{
		return m_bodyA;
	}
	btMultiBody* getMultiBodyB()
	{
		return m_bodyB;
	}

	int getLinkA() const
	{
		return m_linkA;
	}
	int getLinkB() const
	{
		return m_linkB;
	}
	void internalSetAppliedImpulse(int dof, btScalar appliedImpulse)
	{
		btAssert(dof >= 0);
		btAssert(dof < getNumRows());
		m_data[dof] = appliedImpulse;
	}

	btScalar getAppliedImpulse(int dof)
	{
		btAssert(dof >= 0);
		btAssert(dof < getNumRows());
		return m_data[dof];
	}
	
	
	
	btScalar getPosition(int row) const
	{
		return m_data[m_posOffset + row];
	}

	void setPosition(int row, btScalar pos)
	{
		m_data[m_posOffset + row] = pos;
	}

	bool isUnilateral() const
	{
		return m_isUnilateral;
	}

	
	
	
	btScalar* jacobianA(int row)
	{
		return &m_data[m_numRows + row * m_jacSizeBoth];
	}
	const btScalar* jacobianA(int row) const
	{
		return &m_data[m_numRows + (row * m_jacSizeBoth)];
	}
	btScalar* jacobianB(int row)
	{
		return &m_data[m_numRows + (row * m_jacSizeBoth) + m_jacSizeA];
	}
	const btScalar* jacobianB(int row) const
	{
		return &m_data[m_numRows + (row * m_jacSizeBoth) + m_jacSizeA];
	}

	btScalar getMaxAppliedImpulse() const
	{
		return m_maxAppliedImpulse;
	}
	void setMaxAppliedImpulse(btScalar maxImp)
	{
		m_maxAppliedImpulse = maxImp;
	}

	virtual void debugDraw(class btIDebugDraw * drawer) = 0;

	virtual void setGearRatio(btScalar ratio) {}
	virtual void setGearAuxLink(int gearAuxLink) {}
	virtual void setRelativePositionTarget(btScalar relPosTarget) {}
	virtual void setErp(btScalar erp) {}
};

#endif  
