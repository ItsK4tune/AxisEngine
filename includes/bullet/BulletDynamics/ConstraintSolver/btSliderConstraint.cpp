



#include "btSliderConstraint.h"
#include "BulletDynamics/Dynamics/btRigidBody.h"
#include "LinearMath/btTransformUtil.h"
#include <new>

#define USE_OFFSET_FOR_CONSTANT_FRAME true

void btSliderConstraint::initParams()
{
	m_lowerLinLimit = btScalar(1.0);
	m_upperLinLimit = btScalar(-1.0);
	m_lowerAngLimit = btScalar(0.);
	m_upperAngLimit = btScalar(0.);
	m_softnessDirLin = SLIDER_CONSTRAINT_DEF_SOFTNESS;
	m_restitutionDirLin = SLIDER_CONSTRAINT_DEF_RESTITUTION;
	m_dampingDirLin = btScalar(0.);
	m_cfmDirLin = SLIDER_CONSTRAINT_DEF_CFM;
	m_softnessDirAng = SLIDER_CONSTRAINT_DEF_SOFTNESS;
	m_restitutionDirAng = SLIDER_CONSTRAINT_DEF_RESTITUTION;
	m_dampingDirAng = btScalar(0.);
	m_cfmDirAng = SLIDER_CONSTRAINT_DEF_CFM;
	m_softnessOrthoLin = SLIDER_CONSTRAINT_DEF_SOFTNESS;
	m_restitutionOrthoLin = SLIDER_CONSTRAINT_DEF_RESTITUTION;
	m_dampingOrthoLin = SLIDER_CONSTRAINT_DEF_DAMPING;
	m_cfmOrthoLin = SLIDER_CONSTRAINT_DEF_CFM;
	m_softnessOrthoAng = SLIDER_CONSTRAINT_DEF_SOFTNESS;
	m_restitutionOrthoAng = SLIDER_CONSTRAINT_DEF_RESTITUTION;
	m_dampingOrthoAng = SLIDER_CONSTRAINT_DEF_DAMPING;
	m_cfmOrthoAng = SLIDER_CONSTRAINT_DEF_CFM;
	m_softnessLimLin = SLIDER_CONSTRAINT_DEF_SOFTNESS;
	m_restitutionLimLin = SLIDER_CONSTRAINT_DEF_RESTITUTION;
	m_dampingLimLin = SLIDER_CONSTRAINT_DEF_DAMPING;
	m_cfmLimLin = SLIDER_CONSTRAINT_DEF_CFM;
	m_softnessLimAng = SLIDER_CONSTRAINT_DEF_SOFTNESS;
	m_restitutionLimAng = SLIDER_CONSTRAINT_DEF_RESTITUTION;
	m_dampingLimAng = SLIDER_CONSTRAINT_DEF_DAMPING;
	m_cfmLimAng = SLIDER_CONSTRAINT_DEF_CFM;

	m_poweredLinMotor = false;
	m_targetLinMotorVelocity = btScalar(0.);
	m_maxLinMotorForce = btScalar(0.);
	m_accumulatedLinMotorImpulse = btScalar(0.0);

	m_poweredAngMotor = false;
	m_targetAngMotorVelocity = btScalar(0.);
	m_maxAngMotorForce = btScalar(0.);
	m_accumulatedAngMotorImpulse = btScalar(0.0);

	m_flags = 0;
	m_flags = 0;

	m_useOffsetForConstraintFrame = USE_OFFSET_FOR_CONSTANT_FRAME;

	calculateTransforms(m_rbA.getCenterOfMassTransform(), m_rbB.getCenterOfMassTransform());
}

btSliderConstraint::btSliderConstraint(btRigidBody& rbA, btRigidBody& rbB, const btTransform& frameInA, const btTransform& frameInB, bool useLinearReferenceFrameA)
	: btTypedConstraint(SLIDER_CONSTRAINT_TYPE, rbA, rbB),
	  m_useSolveConstraintObsolete(false),
	  m_frameInA(frameInA),
	  m_frameInB(frameInB),
	  m_useLinearReferenceFrameA(useLinearReferenceFrameA)
{
	initParams();
}

btSliderConstraint::btSliderConstraint(btRigidBody& rbB, const btTransform& frameInB, bool useLinearReferenceFrameA)
	: btTypedConstraint(SLIDER_CONSTRAINT_TYPE, getFixedBody(), rbB),
	  m_useSolveConstraintObsolete(false),
	  m_frameInB(frameInB),
	  m_useLinearReferenceFrameA(useLinearReferenceFrameA)
{
	
	m_frameInA = rbB.getCenterOfMassTransform() * m_frameInB;
	

	initParams();
}

void btSliderConstraint::getInfo1(btConstraintInfo1* info)
{
	if (m_useSolveConstraintObsolete)
	{
		info->m_numConstraintRows = 0;
		info->nub = 0;
	}
	else
	{
		info->m_numConstraintRows = 4;  
		info->nub = 2;
		
		calculateTransforms(m_rbA.getCenterOfMassTransform(), m_rbB.getCenterOfMassTransform());
		testAngLimits();
		testLinLimits();
		if (getSolveLinLimit() || getPoweredLinMotor())
		{
			info->m_numConstraintRows++;  
			info->nub--;
		}
		if (getSolveAngLimit() || getPoweredAngMotor())
		{
			info->m_numConstraintRows++;  
			info->nub--;
		}
	}
}

void btSliderConstraint::getInfo1NonVirtual(btConstraintInfo1* info)
{
	info->m_numConstraintRows = 6;  
	info->nub = 0;
}

void btSliderConstraint::getInfo2(btConstraintInfo2* info)
{
	getInfo2NonVirtual(info, m_rbA.getCenterOfMassTransform(), m_rbB.getCenterOfMassTransform(), m_rbA.getLinearVelocity(), m_rbB.getLinearVelocity(), m_rbA.getInvMass(), m_rbB.getInvMass());
}

void btSliderConstraint::calculateTransforms(const btTransform& transA, const btTransform& transB)
{
	if (m_useLinearReferenceFrameA || (!m_useSolveConstraintObsolete))
	{
		m_calculatedTransformA = transA * m_frameInA;
		m_calculatedTransformB = transB * m_frameInB;
	}
	else
	{
		m_calculatedTransformA = transB * m_frameInB;
		m_calculatedTransformB = transA * m_frameInA;
	}
	m_realPivotAInW = m_calculatedTransformA.getOrigin();
	m_realPivotBInW = m_calculatedTransformB.getOrigin();
	m_sliderAxis = m_calculatedTransformA.getBasis().getColumn(0);  
	if (m_useLinearReferenceFrameA || m_useSolveConstraintObsolete)
	{
		m_delta = m_realPivotBInW - m_realPivotAInW;
	}
	else
	{
		m_delta = m_realPivotAInW - m_realPivotBInW;
	}
	m_projPivotInW = m_realPivotAInW + m_sliderAxis.dot(m_delta) * m_sliderAxis;
	btVector3 normalWorld;
	int i;
	
	for (i = 0; i < 3; i++)
	{
		normalWorld = m_calculatedTransformA.getBasis().getColumn(i);
		m_depth[i] = m_delta.dot(normalWorld);
	}
}

void btSliderConstraint::testLinLimits(void)
{
	m_solveLinLim = false;
	m_linPos = m_depth[0];
	if (m_lowerLinLimit <= m_upperLinLimit)
	{
		if (m_depth[0] > m_upperLinLimit)
		{
			m_depth[0] -= m_upperLinLimit;
			m_solveLinLim = true;
		}
		else if (m_depth[0] < m_lowerLinLimit)
		{
			m_depth[0] -= m_lowerLinLimit;
			m_solveLinLim = true;
		}
		else
		{
			m_depth[0] = btScalar(0.);
		}
	}
	else
	{
		m_depth[0] = btScalar(0.);
	}
}

void btSliderConstraint::testAngLimits(void)
{
	m_angDepth = btScalar(0.);
	m_solveAngLim = false;
	if (m_lowerAngLimit <= m_upperAngLimit)
	{
		const btVector3 axisA0 = m_calculatedTransformA.getBasis().getColumn(1);
		const btVector3 axisA1 = m_calculatedTransformA.getBasis().getColumn(2);
		const btVector3 axisB0 = m_calculatedTransformB.getBasis().getColumn(1);
		
		btScalar rot = btAtan2(axisB0.dot(axisA1), axisB0.dot(axisA0));
		rot = btAdjustAngleToLimits(rot, m_lowerAngLimit, m_upperAngLimit);
		m_angPos = rot;
		if (rot < m_lowerAngLimit)
		{
			m_angDepth = rot - m_lowerAngLimit;
			m_solveAngLim = true;
		}
		else if (rot > m_upperAngLimit)
		{
			m_angDepth = rot - m_upperAngLimit;
			m_solveAngLim = true;
		}
	}
}

btVector3 btSliderConstraint::getAncorInA(void)
{
	btVector3 ancorInA;
	ancorInA = m_realPivotAInW + (m_lowerLinLimit + m_upperLinLimit) * btScalar(0.5) * m_sliderAxis;
	ancorInA = m_rbA.getCenterOfMassTransform().inverse() * ancorInA;
	return ancorInA;
}

btVector3 btSliderConstraint::getAncorInB(void)
{
	btVector3 ancorInB;
	ancorInB = m_frameInB.getOrigin();
	return ancorInB;
}

void btSliderConstraint::getInfo2NonVirtual(btConstraintInfo2* info, const btTransform& transA, const btTransform& transB, const btVector3& linVelA, const btVector3& linVelB, btScalar rbAinvMass, btScalar rbBinvMass)
{
	const btTransform& trA = getCalculatedTransformA();
	const btTransform& trB = getCalculatedTransformB();

	btAssert(!m_useSolveConstraintObsolete);
	int i, s = info->rowskip;

	btScalar signFact = m_useLinearReferenceFrameA ? btScalar(1.0f) : btScalar(-1.0f);

	
	btVector3 ofs = trB.getOrigin() - trA.getOrigin();
	
	btScalar miA = rbAinvMass;
	btScalar miB = rbBinvMass;
	bool hasStaticBody = (miA < SIMD_EPSILON) || (miB < SIMD_EPSILON);
	btScalar miS = miA + miB;
	btScalar factA, factB;
	if (miS > btScalar(0.f))
	{
		factA = miB / miS;
	}
	else
	{
		factA = btScalar(0.5f);
	}
	factB = btScalar(1.0f) - factA;
	btVector3 ax1, p, q;
	btVector3 ax1A = trA.getBasis().getColumn(0);
	btVector3 ax1B = trB.getBasis().getColumn(0);
	if (m_useOffsetForConstraintFrame)
	{
		
		
		ax1 = ax1A * factA + ax1B * factB;
		ax1.normalize();
		
		btPlaneSpace1(ax1, p, q);
	}
	else
	{  
		ax1 = trA.getBasis().getColumn(0);
		
		p = trA.getBasis().getColumn(1);
		q = trA.getBasis().getColumn(2);
	}
	
	
	
	
	
	
	
	
	info->m_J1angularAxis[0] = p[0];
	info->m_J1angularAxis[1] = p[1];
	info->m_J1angularAxis[2] = p[2];
	info->m_J1angularAxis[s + 0] = q[0];
	info->m_J1angularAxis[s + 1] = q[1];
	info->m_J1angularAxis[s + 2] = q[2];

	info->m_J2angularAxis[0] = -p[0];
	info->m_J2angularAxis[1] = -p[1];
	info->m_J2angularAxis[2] = -p[2];
	info->m_J2angularAxis[s + 0] = -q[0];
	info->m_J2angularAxis[s + 1] = -q[1];
	info->m_J2angularAxis[s + 2] = -q[2];
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	btScalar currERP = (m_flags & BT_SLIDER_FLAGS_ERP_ORTANG) ? m_softnessOrthoAng : m_softnessOrthoAng * info->erp;
	btScalar k = info->fps * currERP;

	btVector3 u = ax1A.cross(ax1B);
	info->m_constraintError[0] = k * u.dot(p);
	info->m_constraintError[s] = k * u.dot(q);
	if (m_flags & BT_SLIDER_FLAGS_CFM_ORTANG)
	{
		info->cfm[0] = m_cfmOrthoAng;
		info->cfm[s] = m_cfmOrthoAng;
	}

	int nrow = 1;  
	int srow;
	btScalar limit_err;
	int limit;

	
	
	

	btTransform bodyA_trans = transA;
	btTransform bodyB_trans = transB;
	nrow++;
	int s2 = nrow * s;
	nrow++;
	int s3 = nrow * s;
	btVector3 tmpA(0, 0, 0), tmpB(0, 0, 0), relA(0, 0, 0), relB(0, 0, 0), c(0, 0, 0);
	if (m_useOffsetForConstraintFrame)
	{
		
		relB = trB.getOrigin() - bodyB_trans.getOrigin();
		
		btVector3 projB = ax1 * relB.dot(ax1);
		
		btVector3 orthoB = relB - projB;
		
		relA = trA.getOrigin() - bodyA_trans.getOrigin();
		btVector3 projA = ax1 * relA.dot(ax1);
		btVector3 orthoA = relA - projA;
		
		btScalar sliderOffs = m_linPos - m_depth[0];
		
		btVector3 totalDist = projA + ax1 * sliderOffs - projB;
		
		relA = orthoA + totalDist * factA;
		relB = orthoB - totalDist * factB;
		
		p = orthoB * factA + orthoA * factB;
		btScalar len2 = p.length2();
		if (len2 > SIMD_EPSILON)
		{
			p /= btSqrt(len2);
		}
		else
		{
			p = trA.getBasis().getColumn(1);
		}
		
		q = ax1.cross(p);
		
		tmpA = relA.cross(p);
		tmpB = relB.cross(p);
		for (i = 0; i < 3; i++) info->m_J1angularAxis[s2 + i] = tmpA[i];
		for (i = 0; i < 3; i++) info->m_J2angularAxis[s2 + i] = -tmpB[i];
		tmpA = relA.cross(q);
		tmpB = relB.cross(q);
		if (hasStaticBody && getSolveAngLimit())
		{  
			
			tmpB *= factB;
			tmpA *= factA;
		}
		for (i = 0; i < 3; i++) info->m_J1angularAxis[s3 + i] = tmpA[i];
		for (i = 0; i < 3; i++) info->m_J2angularAxis[s3 + i] = -tmpB[i];
		for (i = 0; i < 3; i++) info->m_J1linearAxis[s2 + i] = p[i];
		for (i = 0; i < 3; i++) info->m_J1linearAxis[s3 + i] = q[i];
		for (i = 0; i < 3; i++) info->m_J2linearAxis[s2 + i] = -p[i];
		for (i = 0; i < 3; i++) info->m_J2linearAxis[s3 + i] = -q[i];
	}
	else
	{  
		
		c = bodyB_trans.getOrigin() - bodyA_trans.getOrigin();
		btVector3 tmp = c.cross(p);
		for (i = 0; i < 3; i++) info->m_J1angularAxis[s2 + i] = factA * tmp[i];
		for (i = 0; i < 3; i++) info->m_J2angularAxis[s2 + i] = factB * tmp[i];
		tmp = c.cross(q);
		for (i = 0; i < 3; i++) info->m_J1angularAxis[s3 + i] = factA * tmp[i];
		for (i = 0; i < 3; i++) info->m_J2angularAxis[s3 + i] = factB * tmp[i];

		for (i = 0; i < 3; i++) info->m_J1linearAxis[s2 + i] = p[i];
		for (i = 0; i < 3; i++) info->m_J1linearAxis[s3 + i] = q[i];
		for (i = 0; i < 3; i++) info->m_J2linearAxis[s2 + i] = -p[i];
		for (i = 0; i < 3; i++) info->m_J2linearAxis[s3 + i] = -q[i];
	}
	

	
	currERP = (m_flags & BT_SLIDER_FLAGS_ERP_ORTLIN) ? m_softnessOrthoLin : m_softnessOrthoLin * info->erp;
	k = info->fps * currERP;

	btScalar rhs = k * p.dot(ofs);
	info->m_constraintError[s2] = rhs;
	rhs = k * q.dot(ofs);
	info->m_constraintError[s3] = rhs;
	if (m_flags & BT_SLIDER_FLAGS_CFM_ORTLIN)
	{
		info->cfm[s2] = m_cfmOrthoLin;
		info->cfm[s3] = m_cfmOrthoLin;
	}

	
	limit_err = btScalar(0.0);
	limit = 0;
	if (getSolveLinLimit())
	{
		limit_err = getLinDepth() * signFact;
		limit = (limit_err > btScalar(0.0)) ? 2 : 1;
	}
	bool powered = getPoweredLinMotor();
	
	if (limit || powered)
	{
		nrow++;
		srow = nrow * info->rowskip;
		info->m_J1linearAxis[srow + 0] = ax1[0];
		info->m_J1linearAxis[srow + 1] = ax1[1];
		info->m_J1linearAxis[srow + 2] = ax1[2];
		info->m_J2linearAxis[srow + 0] = -ax1[0];
		info->m_J2linearAxis[srow + 1] = -ax1[1];
		info->m_J2linearAxis[srow + 2] = -ax1[2];
		
		
		
		
		
		
		
		if (m_useOffsetForConstraintFrame)
		{
			
			if (!hasStaticBody)
			{
				tmpA = relA.cross(ax1);
				tmpB = relB.cross(ax1);
				info->m_J1angularAxis[srow + 0] = tmpA[0];
				info->m_J1angularAxis[srow + 1] = tmpA[1];
				info->m_J1angularAxis[srow + 2] = tmpA[2];
				info->m_J2angularAxis[srow + 0] = -tmpB[0];
				info->m_J2angularAxis[srow + 1] = -tmpB[1];
				info->m_J2angularAxis[srow + 2] = -tmpB[2];
			}
		}
		else
		{                   
			btVector3 ltd;  
			ltd = c.cross(ax1);
			info->m_J1angularAxis[srow + 0] = factA * ltd[0];
			info->m_J1angularAxis[srow + 1] = factA * ltd[1];
			info->m_J1angularAxis[srow + 2] = factA * ltd[2];
			info->m_J2angularAxis[srow + 0] = factB * ltd[0];
			info->m_J2angularAxis[srow + 1] = factB * ltd[1];
			info->m_J2angularAxis[srow + 2] = factB * ltd[2];
		}
		
		btScalar lostop = getLowerLinLimit();
		btScalar histop = getUpperLinLimit();
		if (limit && (lostop == histop))
		{  
			powered = false;
		}
		info->m_constraintError[srow] = 0.;
		info->m_lowerLimit[srow] = 0.;
		info->m_upperLimit[srow] = 0.;
		currERP = (m_flags & BT_SLIDER_FLAGS_ERP_LIMLIN) ? m_softnessLimLin : info->erp;
		if (powered)
		{
			if (m_flags & BT_SLIDER_FLAGS_CFM_DIRLIN)
			{
				info->cfm[srow] = m_cfmDirLin;
			}
			btScalar tag_vel = getTargetLinMotorVelocity();
			btScalar mot_fact = getMotorFactor(m_linPos, m_lowerLinLimit, m_upperLinLimit, tag_vel, info->fps * currERP);
			info->m_constraintError[srow] -= signFact * mot_fact * getTargetLinMotorVelocity();
			info->m_lowerLimit[srow] += -getMaxLinMotorForce() / info->fps;
			info->m_upperLimit[srow] += getMaxLinMotorForce() / info->fps;
		}
		if (limit)
		{
			k = info->fps * currERP;
			info->m_constraintError[srow] += k * limit_err;
			if (m_flags & BT_SLIDER_FLAGS_CFM_LIMLIN)
			{
				info->cfm[srow] = m_cfmLimLin;
			}
			if (lostop == histop)
			{  
				info->m_lowerLimit[srow] = -SIMD_INFINITY;
				info->m_upperLimit[srow] = SIMD_INFINITY;
			}
			else if (limit == 1)
			{  
				info->m_lowerLimit[srow] = -SIMD_INFINITY;
				info->m_upperLimit[srow] = 0;
			}
			else
			{  
				info->m_lowerLimit[srow] = 0;
				info->m_upperLimit[srow] = SIMD_INFINITY;
			}
			
			btScalar bounce = btFabs(btScalar(1.0) - getDampingLimLin());
			if (bounce > btScalar(0.0))
			{
				btScalar vel = linVelA.dot(ax1);
				vel -= linVelB.dot(ax1);
				vel *= signFact;
				
				
				if (limit == 1)
				{  
					if (vel < 0)
					{
						btScalar newc = -bounce * vel;
						if (newc > info->m_constraintError[srow])
						{
							info->m_constraintError[srow] = newc;
						}
					}
				}
				else
				{  
					if (vel > 0)
					{
						btScalar newc = -bounce * vel;
						if (newc < info->m_constraintError[srow])
						{
							info->m_constraintError[srow] = newc;
						}
					}
				}
			}
			info->m_constraintError[srow] *= getSoftnessLimLin();
		}  
	}      
	
	limit_err = btScalar(0.0);
	limit = 0;
	if (getSolveAngLimit())
	{
		limit_err = getAngDepth();
		limit = (limit_err > btScalar(0.0)) ? 1 : 2;
	}
	
	powered = getPoweredAngMotor();
	if (limit || powered)
	{
		nrow++;
		srow = nrow * info->rowskip;
		info->m_J1angularAxis[srow + 0] = ax1[0];
		info->m_J1angularAxis[srow + 1] = ax1[1];
		info->m_J1angularAxis[srow + 2] = ax1[2];

		info->m_J2angularAxis[srow + 0] = -ax1[0];
		info->m_J2angularAxis[srow + 1] = -ax1[1];
		info->m_J2angularAxis[srow + 2] = -ax1[2];

		btScalar lostop = getLowerAngLimit();
		btScalar histop = getUpperAngLimit();
		if (limit && (lostop == histop))
		{  
			powered = false;
		}
		currERP = (m_flags & BT_SLIDER_FLAGS_ERP_LIMANG) ? m_softnessLimAng : info->erp;
		if (powered)
		{
			if (m_flags & BT_SLIDER_FLAGS_CFM_DIRANG)
			{
				info->cfm[srow] = m_cfmDirAng;
			}
			btScalar mot_fact = getMotorFactor(m_angPos, m_lowerAngLimit, m_upperAngLimit, getTargetAngMotorVelocity(), info->fps * currERP);
			info->m_constraintError[srow] = mot_fact * getTargetAngMotorVelocity();
			info->m_lowerLimit[srow] = -getMaxAngMotorForce() / info->fps;
			info->m_upperLimit[srow] = getMaxAngMotorForce() / info->fps;
		}
		if (limit)
		{
			k = info->fps * currERP;
			info->m_constraintError[srow] += k * limit_err;
			if (m_flags & BT_SLIDER_FLAGS_CFM_LIMANG)
			{
				info->cfm[srow] = m_cfmLimAng;
			}
			if (lostop == histop)
			{
				
				info->m_lowerLimit[srow] = -SIMD_INFINITY;
				info->m_upperLimit[srow] = SIMD_INFINITY;
			}
			else if (limit == 1)
			{  
				info->m_lowerLimit[srow] = 0;
				info->m_upperLimit[srow] = SIMD_INFINITY;
			}
			else
			{  
				info->m_lowerLimit[srow] = -SIMD_INFINITY;
				info->m_upperLimit[srow] = 0;
			}
			
			btScalar bounce = btFabs(btScalar(1.0) - getDampingLimAng());
			if (bounce > btScalar(0.0))
			{
				btScalar vel = m_rbA.getAngularVelocity().dot(ax1);
				vel -= m_rbB.getAngularVelocity().dot(ax1);
				
				
				if (limit == 1)
				{  
					if (vel < 0)
					{
						btScalar newc = -bounce * vel;
						if (newc > info->m_constraintError[srow])
						{
							info->m_constraintError[srow] = newc;
						}
					}
				}
				else
				{  
					if (vel > 0)
					{
						btScalar newc = -bounce * vel;
						if (newc < info->m_constraintError[srow])
						{
							info->m_constraintError[srow] = newc;
						}
					}
				}
			}
			info->m_constraintError[srow] *= getSoftnessLimAng();
		}  
	}      
}



void btSliderConstraint::setParam(int num, btScalar value, int axis)
{
	switch (num)
	{
		case BT_CONSTRAINT_STOP_ERP:
			if (axis < 1)
			{
				m_softnessLimLin = value;
				m_flags |= BT_SLIDER_FLAGS_ERP_LIMLIN;
			}
			else if (axis < 3)
			{
				m_softnessOrthoLin = value;
				m_flags |= BT_SLIDER_FLAGS_ERP_ORTLIN;
			}
			else if (axis == 3)
			{
				m_softnessLimAng = value;
				m_flags |= BT_SLIDER_FLAGS_ERP_LIMANG;
			}
			else if (axis < 6)
			{
				m_softnessOrthoAng = value;
				m_flags |= BT_SLIDER_FLAGS_ERP_ORTANG;
			}
			else
			{
				btAssertConstrParams(0);
			}
			break;
		case BT_CONSTRAINT_CFM:
			if (axis < 1)
			{
				m_cfmDirLin = value;
				m_flags |= BT_SLIDER_FLAGS_CFM_DIRLIN;
			}
			else if (axis == 3)
			{
				m_cfmDirAng = value;
				m_flags |= BT_SLIDER_FLAGS_CFM_DIRANG;
			}
			else
			{
				btAssertConstrParams(0);
			}
			break;
		case BT_CONSTRAINT_STOP_CFM:
			if (axis < 1)
			{
				m_cfmLimLin = value;
				m_flags |= BT_SLIDER_FLAGS_CFM_LIMLIN;
			}
			else if (axis < 3)
			{
				m_cfmOrthoLin = value;
				m_flags |= BT_SLIDER_FLAGS_CFM_ORTLIN;
			}
			else if (axis == 3)
			{
				m_cfmLimAng = value;
				m_flags |= BT_SLIDER_FLAGS_CFM_LIMANG;
			}
			else if (axis < 6)
			{
				m_cfmOrthoAng = value;
				m_flags |= BT_SLIDER_FLAGS_CFM_ORTANG;
			}
			else
			{
				btAssertConstrParams(0);
			}
			break;
	}
}


btScalar btSliderConstraint::getParam(int num, int axis) const
{
	btScalar retVal(SIMD_INFINITY);
	switch (num)
	{
		case BT_CONSTRAINT_STOP_ERP:
			if (axis < 1)
			{
				btAssertConstrParams(m_flags & BT_SLIDER_FLAGS_ERP_LIMLIN);
				retVal = m_softnessLimLin;
			}
			else if (axis < 3)
			{
				btAssertConstrParams(m_flags & BT_SLIDER_FLAGS_ERP_ORTLIN);
				retVal = m_softnessOrthoLin;
			}
			else if (axis == 3)
			{
				btAssertConstrParams(m_flags & BT_SLIDER_FLAGS_ERP_LIMANG);
				retVal = m_softnessLimAng;
			}
			else if (axis < 6)
			{
				btAssertConstrParams(m_flags & BT_SLIDER_FLAGS_ERP_ORTANG);
				retVal = m_softnessOrthoAng;
			}
			else
			{
				btAssertConstrParams(0);
			}
			break;
		case BT_CONSTRAINT_CFM:
			if (axis < 1)
			{
				btAssertConstrParams(m_flags & BT_SLIDER_FLAGS_CFM_DIRLIN);
				retVal = m_cfmDirLin;
			}
			else if (axis == 3)
			{
				btAssertConstrParams(m_flags & BT_SLIDER_FLAGS_CFM_DIRANG);
				retVal = m_cfmDirAng;
			}
			else
			{
				btAssertConstrParams(0);
			}
			break;
		case BT_CONSTRAINT_STOP_CFM:
			if (axis < 1)
			{
				btAssertConstrParams(m_flags & BT_SLIDER_FLAGS_CFM_LIMLIN);
				retVal = m_cfmLimLin;
			}
			else if (axis < 3)
			{
				btAssertConstrParams(m_flags & BT_SLIDER_FLAGS_CFM_ORTLIN);
				retVal = m_cfmOrthoLin;
			}
			else if (axis == 3)
			{
				btAssertConstrParams(m_flags & BT_SLIDER_FLAGS_CFM_LIMANG);
				retVal = m_cfmLimAng;
			}
			else if (axis < 6)
			{
				btAssertConstrParams(m_flags & BT_SLIDER_FLAGS_CFM_ORTANG);
				retVal = m_cfmOrthoAng;
			}
			else
			{
				btAssertConstrParams(0);
			}
			break;
	}
	return retVal;
}
