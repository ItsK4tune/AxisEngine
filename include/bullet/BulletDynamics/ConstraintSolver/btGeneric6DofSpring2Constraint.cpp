








#include "btGeneric6DofSpring2Constraint.h"
#include "BulletDynamics/Dynamics/btRigidBody.h"
#include "LinearMath/btTransformUtil.h"
#include <cmath>
#include <new>

btGeneric6DofSpring2Constraint::btGeneric6DofSpring2Constraint(btRigidBody& rbA, btRigidBody& rbB, const btTransform& frameInA, const btTransform& frameInB, RotateOrder rotOrder)
	: btTypedConstraint(D6_SPRING_2_CONSTRAINT_TYPE, rbA, rbB), m_frameInA(frameInA), m_frameInB(frameInB), m_rotateOrder(rotOrder), m_flags(0)
{
	calculateTransforms();
}

btGeneric6DofSpring2Constraint::btGeneric6DofSpring2Constraint(btRigidBody& rbB, const btTransform& frameInB, RotateOrder rotOrder)
	: btTypedConstraint(D6_SPRING_2_CONSTRAINT_TYPE, getFixedBody(), rbB), m_frameInB(frameInB), m_rotateOrder(rotOrder), m_flags(0)
{
	
	m_frameInA = rbB.getCenterOfMassTransform() * m_frameInB;
	calculateTransforms();
}

btScalar btGeneric6DofSpring2Constraint::btGetMatrixElem(const btMatrix3x3& mat, int index)
{
	int i = index % 3;
	int j = index / 3;
	return mat[i][j];
}



bool btGeneric6DofSpring2Constraint::matrixToEulerXYZ(const btMatrix3x3& mat, btVector3& xyz)
{
	
	
	

	btScalar fi = btGetMatrixElem(mat, 2);
	if (fi < btScalar(1.0f))
	{
		if (fi > btScalar(-1.0f))
		{
			xyz[0] = btAtan2(-btGetMatrixElem(mat, 5), btGetMatrixElem(mat, 8));
			xyz[1] = btAsin(btGetMatrixElem(mat, 2));
			xyz[2] = btAtan2(-btGetMatrixElem(mat, 1), btGetMatrixElem(mat, 0));
			return true;
		}
		else
		{
			
			xyz[0] = -btAtan2(btGetMatrixElem(mat, 3), btGetMatrixElem(mat, 4));
			xyz[1] = -SIMD_HALF_PI;
			xyz[2] = btScalar(0.0);
			return false;
		}
	}
	else
	{
		
		xyz[0] = btAtan2(btGetMatrixElem(mat, 3), btGetMatrixElem(mat, 4));
		xyz[1] = SIMD_HALF_PI;
		xyz[2] = 0.0;
	}
	return false;
}

bool btGeneric6DofSpring2Constraint::matrixToEulerXZY(const btMatrix3x3& mat, btVector3& xyz)
{
	
	
	

	btScalar fi = btGetMatrixElem(mat, 1);
	if (fi < btScalar(1.0f))
	{
		if (fi > btScalar(-1.0f))
		{
			xyz[0] = btAtan2(btGetMatrixElem(mat, 7), btGetMatrixElem(mat, 4));
			xyz[1] = btAtan2(btGetMatrixElem(mat, 2), btGetMatrixElem(mat, 0));
			xyz[2] = btAsin(-btGetMatrixElem(mat, 1));
			return true;
		}
		else
		{
			xyz[0] = -btAtan2(-btGetMatrixElem(mat, 6), btGetMatrixElem(mat, 8));
			xyz[1] = btScalar(0.0);
			xyz[2] = SIMD_HALF_PI;
			return false;
		}
	}
	else
	{
		xyz[0] = btAtan2(-btGetMatrixElem(mat, 6), btGetMatrixElem(mat, 8));
		xyz[1] = 0.0;
		xyz[2] = -SIMD_HALF_PI;
	}
	return false;
}

bool btGeneric6DofSpring2Constraint::matrixToEulerYXZ(const btMatrix3x3& mat, btVector3& xyz)
{
	
	
	

	btScalar fi = btGetMatrixElem(mat, 5);
	if (fi < btScalar(1.0f))
	{
		if (fi > btScalar(-1.0f))
		{
			xyz[0] = btAsin(-btGetMatrixElem(mat, 5));
			xyz[1] = btAtan2(btGetMatrixElem(mat, 2), btGetMatrixElem(mat, 8));
			xyz[2] = btAtan2(btGetMatrixElem(mat, 3), btGetMatrixElem(mat, 4));
			return true;
		}
		else
		{
			xyz[0] = SIMD_HALF_PI;
			xyz[1] = -btAtan2(-btGetMatrixElem(mat, 1), btGetMatrixElem(mat, 0));
			xyz[2] = btScalar(0.0);
			return false;
		}
	}
	else
	{
		xyz[0] = -SIMD_HALF_PI;
		xyz[1] = btAtan2(-btGetMatrixElem(mat, 1), btGetMatrixElem(mat, 0));
		xyz[2] = 0.0;
	}
	return false;
}

bool btGeneric6DofSpring2Constraint::matrixToEulerYZX(const btMatrix3x3& mat, btVector3& xyz)
{
	
	
	

	btScalar fi = btGetMatrixElem(mat, 3);
	if (fi < btScalar(1.0f))
	{
		if (fi > btScalar(-1.0f))
		{
			xyz[0] = btAtan2(-btGetMatrixElem(mat, 5), btGetMatrixElem(mat, 4));
			xyz[1] = btAtan2(-btGetMatrixElem(mat, 6), btGetMatrixElem(mat, 0));
			xyz[2] = btAsin(btGetMatrixElem(mat, 3));
			return true;
		}
		else
		{
			xyz[0] = btScalar(0.0);
			xyz[1] = -btAtan2(btGetMatrixElem(mat, 7), btGetMatrixElem(mat, 8));
			xyz[2] = -SIMD_HALF_PI;
			return false;
		}
	}
	else
	{
		xyz[0] = btScalar(0.0);
		xyz[1] = btAtan2(btGetMatrixElem(mat, 7), btGetMatrixElem(mat, 8));
		xyz[2] = SIMD_HALF_PI;
	}
	return false;
}

bool btGeneric6DofSpring2Constraint::matrixToEulerZXY(const btMatrix3x3& mat, btVector3& xyz)
{
	
	
	

	btScalar fi = btGetMatrixElem(mat, 7);
	if (fi < btScalar(1.0f))
	{
		if (fi > btScalar(-1.0f))
		{
			xyz[0] = btAsin(btGetMatrixElem(mat, 7));
			xyz[1] = btAtan2(-btGetMatrixElem(mat, 6), btGetMatrixElem(mat, 8));
			xyz[2] = btAtan2(-btGetMatrixElem(mat, 1), btGetMatrixElem(mat, 4));
			return true;
		}
		else
		{
			xyz[0] = -SIMD_HALF_PI;
			xyz[1] = btScalar(0.0);
			xyz[2] = -btAtan2(btGetMatrixElem(mat, 2), btGetMatrixElem(mat, 0));
			return false;
		}
	}
	else
	{
		xyz[0] = SIMD_HALF_PI;
		xyz[1] = btScalar(0.0);
		xyz[2] = btAtan2(btGetMatrixElem(mat, 2), btGetMatrixElem(mat, 0));
	}
	return false;
}

bool btGeneric6DofSpring2Constraint::matrixToEulerZYX(const btMatrix3x3& mat, btVector3& xyz)
{
	
	
	

	btScalar fi = btGetMatrixElem(mat, 6);
	if (fi < btScalar(1.0f))
	{
		if (fi > btScalar(-1.0f))
		{
			xyz[0] = btAtan2(btGetMatrixElem(mat, 7), btGetMatrixElem(mat, 8));
			xyz[1] = btAsin(-btGetMatrixElem(mat, 6));
			xyz[2] = btAtan2(btGetMatrixElem(mat, 3), btGetMatrixElem(mat, 0));
			return true;
		}
		else
		{
			xyz[0] = btScalar(0.0);
			xyz[1] = SIMD_HALF_PI;
			xyz[2] = -btAtan2(btGetMatrixElem(mat, 1), btGetMatrixElem(mat, 2));
			return false;
		}
	}
	else
	{
		xyz[0] = btScalar(0.0);
		xyz[1] = -SIMD_HALF_PI;
		xyz[2] = btAtan2(-btGetMatrixElem(mat, 1), -btGetMatrixElem(mat, 2));
	}
	return false;
}

void btGeneric6DofSpring2Constraint::calculateAngleInfo()
{
	btMatrix3x3 relative_frame = m_calculatedTransformA.getBasis().inverse() * m_calculatedTransformB.getBasis();
	switch (m_rotateOrder)
	{
		case RO_XYZ:
			matrixToEulerXYZ(relative_frame, m_calculatedAxisAngleDiff);
			break;
		case RO_XZY:
			matrixToEulerXZY(relative_frame, m_calculatedAxisAngleDiff);
			break;
		case RO_YXZ:
			matrixToEulerYXZ(relative_frame, m_calculatedAxisAngleDiff);
			break;
		case RO_YZX:
			matrixToEulerYZX(relative_frame, m_calculatedAxisAngleDiff);
			break;
		case RO_ZXY:
			matrixToEulerZXY(relative_frame, m_calculatedAxisAngleDiff);
			break;
		case RO_ZYX:
			matrixToEulerZYX(relative_frame, m_calculatedAxisAngleDiff);
			break;
		default:
			btAssert(false);
	}
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	switch (m_rotateOrder)
	{
		case RO_XYZ:
		{
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			btVector3 axis0 = m_calculatedTransformB.getBasis().getColumn(0);
			btVector3 axis2 = m_calculatedTransformA.getBasis().getColumn(2);
			m_calculatedAxis[1] = axis2.cross(axis0);
			m_calculatedAxis[0] = m_calculatedAxis[1].cross(axis2);
			m_calculatedAxis[2] = axis0.cross(m_calculatedAxis[1]);
			break;
		}
		case RO_XZY:
		{
			
			
			
			
			btVector3 axis0 = m_calculatedTransformB.getBasis().getColumn(0);
			btVector3 axis1 = m_calculatedTransformA.getBasis().getColumn(1);
			m_calculatedAxis[2] = axis0.cross(axis1);
			m_calculatedAxis[0] = axis1.cross(m_calculatedAxis[2]);
			m_calculatedAxis[1] = m_calculatedAxis[2].cross(axis0);
			break;
		}
		case RO_YXZ:
		{
			
			
			
			
			btVector3 axis1 = m_calculatedTransformB.getBasis().getColumn(1);
			btVector3 axis2 = m_calculatedTransformA.getBasis().getColumn(2);
			m_calculatedAxis[0] = axis1.cross(axis2);
			m_calculatedAxis[1] = axis2.cross(m_calculatedAxis[0]);
			m_calculatedAxis[2] = m_calculatedAxis[0].cross(axis1);
			break;
		}
		case RO_YZX:
		{
			
			
			
			
			btVector3 axis0 = m_calculatedTransformA.getBasis().getColumn(0);
			btVector3 axis1 = m_calculatedTransformB.getBasis().getColumn(1);
			m_calculatedAxis[2] = axis0.cross(axis1);
			m_calculatedAxis[0] = axis1.cross(m_calculatedAxis[2]);
			m_calculatedAxis[1] = m_calculatedAxis[2].cross(axis0);
			break;
		}
		case RO_ZXY:
		{
			
			
			
			
			btVector3 axis1 = m_calculatedTransformA.getBasis().getColumn(1);
			btVector3 axis2 = m_calculatedTransformB.getBasis().getColumn(2);
			m_calculatedAxis[0] = axis1.cross(axis2);
			m_calculatedAxis[1] = axis2.cross(m_calculatedAxis[0]);
			m_calculatedAxis[2] = m_calculatedAxis[0].cross(axis1);
			break;
		}
		case RO_ZYX:
		{
			
			
			
			
			btVector3 axis0 = m_calculatedTransformA.getBasis().getColumn(0);
			btVector3 axis2 = m_calculatedTransformB.getBasis().getColumn(2);
			m_calculatedAxis[1] = axis2.cross(axis0);
			m_calculatedAxis[0] = m_calculatedAxis[1].cross(axis2);
			m_calculatedAxis[2] = axis0.cross(m_calculatedAxis[1]);
			break;
		}
		default:
			btAssert(false);
	}

	m_calculatedAxis[0].normalize();
	m_calculatedAxis[1].normalize();
	m_calculatedAxis[2].normalize();
}

void btGeneric6DofSpring2Constraint::calculateTransforms()
{
	calculateTransforms(m_rbA.getCenterOfMassTransform(), m_rbB.getCenterOfMassTransform());
}

void btGeneric6DofSpring2Constraint::calculateTransforms(const btTransform& transA, const btTransform& transB)
{
	m_calculatedTransformA = transA * m_frameInA;
	m_calculatedTransformB = transB * m_frameInB;
	calculateLinearInfo();
	calculateAngleInfo();

	btScalar miA = getRigidBodyA().getInvMass();
	btScalar miB = getRigidBodyB().getInvMass();
	m_hasStaticBody = (miA < SIMD_EPSILON) || (miB < SIMD_EPSILON);
	btScalar miS = miA + miB;
	if (miS > btScalar(0.f))
	{
		m_factA = miB / miS;
	}
	else
	{
		m_factA = btScalar(0.5f);
	}
	m_factB = btScalar(1.0f) - m_factA;
}

void btGeneric6DofSpring2Constraint::testAngularLimitMotor(int axis_index)
{
	btScalar angle = m_calculatedAxisAngleDiff[axis_index];
	angle = btAdjustAngleToLimits(angle, m_angularLimits[axis_index].m_loLimit, m_angularLimits[axis_index].m_hiLimit);
	m_angularLimits[axis_index].m_currentPosition = angle;
	m_angularLimits[axis_index].testLimitValue(angle);
}

void btGeneric6DofSpring2Constraint::getInfo1(btConstraintInfo1* info)
{
	
	calculateTransforms(m_rbA.getCenterOfMassTransform(), m_rbB.getCenterOfMassTransform());
	info->m_numConstraintRows = 0;
	info->nub = 0;
	int i;
	
	for (i = 0; i < 3; i++)
	{
		if (m_linearLimits.m_currentLimit[i] == 4)
			info->m_numConstraintRows += 2;
		else if (m_linearLimits.m_currentLimit[i] != 0)
			info->m_numConstraintRows += 1;
		if (m_linearLimits.m_enableMotor[i]) info->m_numConstraintRows += 1;
		if (m_linearLimits.m_enableSpring[i]) info->m_numConstraintRows += 1;
	}
	
	for (i = 0; i < 3; i++)
	{
		testAngularLimitMotor(i);
		if (m_angularLimits[i].m_currentLimit == 4)
			info->m_numConstraintRows += 2;
		else if (m_angularLimits[i].m_currentLimit != 0)
			info->m_numConstraintRows += 1;
		if (m_angularLimits[i].m_enableMotor) info->m_numConstraintRows += 1;
		if (m_angularLimits[i].m_enableSpring) info->m_numConstraintRows += 1;
	}
}

void btGeneric6DofSpring2Constraint::getInfo2(btConstraintInfo2* info)
{
	const btTransform& transA = m_rbA.getCenterOfMassTransform();
	const btTransform& transB = m_rbB.getCenterOfMassTransform();
	const btVector3& linVelA = m_rbA.getLinearVelocity();
	const btVector3& linVelB = m_rbB.getLinearVelocity();
	const btVector3& angVelA = m_rbA.getAngularVelocity();
	const btVector3& angVelB = m_rbB.getAngularVelocity();

	
	int row = setAngularLimits(info, 0, transA, transB, linVelA, linVelB, angVelA, angVelB);
	setLinearLimits(info, row, transA, transB, linVelA, linVelB, angVelA, angVelB);
}

int btGeneric6DofSpring2Constraint::setLinearLimits(btConstraintInfo2* info, int row, const btTransform& transA, const btTransform& transB, const btVector3& linVelA, const btVector3& linVelB, const btVector3& angVelA, const btVector3& angVelB)
{
	
	btRotationalLimitMotor2 limot;
	for (int i = 0; i < 3; i++)
	{
		if (m_linearLimits.m_currentLimit[i] || m_linearLimits.m_enableMotor[i] || m_linearLimits.m_enableSpring[i])
		{  
			limot.m_bounce = m_linearLimits.m_bounce[i];
			limot.m_currentLimit = m_linearLimits.m_currentLimit[i];
			limot.m_currentPosition = m_linearLimits.m_currentLinearDiff[i];
			limot.m_currentLimitError = m_linearLimits.m_currentLimitError[i];
			limot.m_currentLimitErrorHi = m_linearLimits.m_currentLimitErrorHi[i];
			limot.m_enableMotor = m_linearLimits.m_enableMotor[i];
			limot.m_servoMotor = m_linearLimits.m_servoMotor[i];
			limot.m_servoTarget = m_linearLimits.m_servoTarget[i];
			limot.m_enableSpring = m_linearLimits.m_enableSpring[i];
			limot.m_springStiffness = m_linearLimits.m_springStiffness[i];
			limot.m_springStiffnessLimited = m_linearLimits.m_springStiffnessLimited[i];
			limot.m_springDamping = m_linearLimits.m_springDamping[i];
			limot.m_springDampingLimited = m_linearLimits.m_springDampingLimited[i];
			limot.m_equilibriumPoint = m_linearLimits.m_equilibriumPoint[i];
			limot.m_hiLimit = m_linearLimits.m_upperLimit[i];
			limot.m_loLimit = m_linearLimits.m_lowerLimit[i];
			limot.m_maxMotorForce = m_linearLimits.m_maxMotorForce[i];
			limot.m_targetVelocity = m_linearLimits.m_targetVelocity[i];
			btVector3 axis = m_calculatedTransformA.getBasis().getColumn(i);
			int flags = m_flags >> (i * BT_6DOF_FLAGS_AXIS_SHIFT2);
			limot.m_stopCFM = (flags & BT_6DOF_FLAGS_CFM_STOP2) ? m_linearLimits.m_stopCFM[i] : info->cfm[0];
			limot.m_stopERP = (flags & BT_6DOF_FLAGS_ERP_STOP2) ? m_linearLimits.m_stopERP[i] : info->erp;
			limot.m_motorCFM = (flags & BT_6DOF_FLAGS_CFM_MOTO2) ? m_linearLimits.m_motorCFM[i] : info->cfm[0];
			limot.m_motorERP = (flags & BT_6DOF_FLAGS_ERP_MOTO2) ? m_linearLimits.m_motorERP[i] : info->erp;

			
			int indx1 = (i + 1) % 3;
			int indx2 = (i + 2) % 3;
			int rotAllowed = 1;  
#define D6_LIMIT_ERROR_THRESHOLD_FOR_ROTATION 1.0e-3
			bool indx1Violated = m_angularLimits[indx1].m_currentLimit == 1 ||
								 m_angularLimits[indx1].m_currentLimit == 2 ||
								 (m_angularLimits[indx1].m_currentLimit == 3 && (m_angularLimits[indx1].m_currentLimitError < -D6_LIMIT_ERROR_THRESHOLD_FOR_ROTATION || m_angularLimits[indx1].m_currentLimitError > D6_LIMIT_ERROR_THRESHOLD_FOR_ROTATION)) ||
								 (m_angularLimits[indx1].m_currentLimit == 4 && (m_angularLimits[indx1].m_currentLimitError < -D6_LIMIT_ERROR_THRESHOLD_FOR_ROTATION || m_angularLimits[indx1].m_currentLimitErrorHi > D6_LIMIT_ERROR_THRESHOLD_FOR_ROTATION));
			bool indx2Violated = m_angularLimits[indx2].m_currentLimit == 1 ||
								 m_angularLimits[indx2].m_currentLimit == 2 ||
								 (m_angularLimits[indx2].m_currentLimit == 3 && (m_angularLimits[indx2].m_currentLimitError < -D6_LIMIT_ERROR_THRESHOLD_FOR_ROTATION || m_angularLimits[indx2].m_currentLimitError > D6_LIMIT_ERROR_THRESHOLD_FOR_ROTATION)) ||
								 (m_angularLimits[indx2].m_currentLimit == 4 && (m_angularLimits[indx2].m_currentLimitError < -D6_LIMIT_ERROR_THRESHOLD_FOR_ROTATION || m_angularLimits[indx2].m_currentLimitErrorHi > D6_LIMIT_ERROR_THRESHOLD_FOR_ROTATION));
			if (indx1Violated && indx2Violated)
			{
				rotAllowed = 0;
			}
			row += get_limit_motor_info2(&limot, transA, transB, linVelA, linVelB, angVelA, angVelB, info, row, axis, 0, rotAllowed);
		}
	}
	return row;
}

int btGeneric6DofSpring2Constraint::setAngularLimits(btConstraintInfo2* info, int row_offset, const btTransform& transA, const btTransform& transB, const btVector3& linVelA, const btVector3& linVelB, const btVector3& angVelA, const btVector3& angVelB)
{
	int row = row_offset;

	
	int cIdx[] = {0, 1, 2};
	switch (m_rotateOrder)
	{
		case RO_XYZ:
			cIdx[0] = 0;
			cIdx[1] = 1;
			cIdx[2] = 2;
			break;
		case RO_XZY:
			cIdx[0] = 0;
			cIdx[1] = 2;
			cIdx[2] = 1;
			break;
		case RO_YXZ:
			cIdx[0] = 1;
			cIdx[1] = 0;
			cIdx[2] = 2;
			break;
		case RO_YZX:
			cIdx[0] = 1;
			cIdx[1] = 2;
			cIdx[2] = 0;
			break;
		case RO_ZXY:
			cIdx[0] = 2;
			cIdx[1] = 0;
			cIdx[2] = 1;
			break;
		case RO_ZYX:
			cIdx[0] = 2;
			cIdx[1] = 1;
			cIdx[2] = 0;
			break;
		default:
			btAssert(false);
	}

	for (int ii = 0; ii < 3; ii++)
	{
		int i = cIdx[ii];
		if (m_angularLimits[i].m_currentLimit || m_angularLimits[i].m_enableMotor || m_angularLimits[i].m_enableSpring)
		{
			btVector3 axis = getAxis(i);
			int flags = m_flags >> ((i + 3) * BT_6DOF_FLAGS_AXIS_SHIFT2);
			if (!(flags & BT_6DOF_FLAGS_CFM_STOP2))
			{
				m_angularLimits[i].m_stopCFM = info->cfm[0];
			}
			if (!(flags & BT_6DOF_FLAGS_ERP_STOP2))
			{
				m_angularLimits[i].m_stopERP = info->erp;
			}
			if (!(flags & BT_6DOF_FLAGS_CFM_MOTO2))
			{
				m_angularLimits[i].m_motorCFM = info->cfm[0];
			}
			if (!(flags & BT_6DOF_FLAGS_ERP_MOTO2))
			{
				m_angularLimits[i].m_motorERP = info->erp;
			}
			row += get_limit_motor_info2(&m_angularLimits[i], transA, transB, linVelA, linVelB, angVelA, angVelB, info, row, axis, 1);
		}
	}

	return row;
}

void btGeneric6DofSpring2Constraint::setFrames(const btTransform& frameA, const btTransform& frameB)
{
	m_frameInA = frameA;
	m_frameInB = frameB;
	buildJacobian();
	calculateTransforms();
}

void btGeneric6DofSpring2Constraint::calculateLinearInfo()
{
	m_calculatedLinearDiff = m_calculatedTransformB.getOrigin() - m_calculatedTransformA.getOrigin();
	m_calculatedLinearDiff = m_calculatedTransformA.getBasis().inverse() * m_calculatedLinearDiff;
	for (int i = 0; i < 3; i++)
	{
		m_linearLimits.m_currentLinearDiff[i] = m_calculatedLinearDiff[i];
		m_linearLimits.testLimitValue(i, m_calculatedLinearDiff[i]);
	}
}

void btGeneric6DofSpring2Constraint::calculateJacobi(btRotationalLimitMotor2* limot, const btTransform& transA, const btTransform& transB, btConstraintInfo2* info, int srow, btVector3& ax1, int rotational, int rotAllowed)
{
	btScalar* J1 = rotational ? info->m_J1angularAxis : info->m_J1linearAxis;
	btScalar* J2 = rotational ? info->m_J2angularAxis : info->m_J2linearAxis;

	J1[srow + 0] = ax1[0];
	J1[srow + 1] = ax1[1];
	J1[srow + 2] = ax1[2];

	J2[srow + 0] = -ax1[0];
	J2[srow + 1] = -ax1[1];
	J2[srow + 2] = -ax1[2];

	if (!rotational)
	{
		btVector3 tmpA, tmpB, relA, relB;
		
		relB = m_calculatedTransformB.getOrigin() - transB.getOrigin();
		
		relA = m_calculatedTransformA.getOrigin() - transA.getOrigin();
		tmpA = relA.cross(ax1);
		tmpB = relB.cross(ax1);
		if (m_hasStaticBody && (!rotAllowed))
		{
			tmpA *= m_factA;
			tmpB *= m_factB;
		}
		int i;
		for (i = 0; i < 3; i++) info->m_J1angularAxis[srow + i] = tmpA[i];
		for (i = 0; i < 3; i++) info->m_J2angularAxis[srow + i] = -tmpB[i];
	}
}

int btGeneric6DofSpring2Constraint::get_limit_motor_info2(
	btRotationalLimitMotor2* limot,
	const btTransform& transA, const btTransform& transB, const btVector3& linVelA, const btVector3& linVelB, const btVector3& angVelA, const btVector3& angVelB,
	btConstraintInfo2* info, int row, btVector3& ax1, int rotational, int rotAllowed)
{
	int count = 0;
	int srow = row * info->rowskip;

	if (limot->m_currentLimit == 4)
	{
		btScalar vel = rotational ? angVelA.dot(ax1) - angVelB.dot(ax1) : linVelA.dot(ax1) - linVelB.dot(ax1);

		calculateJacobi(limot, transA, transB, info, srow, ax1, rotational, rotAllowed);
		info->m_constraintError[srow] = info->fps * limot->m_stopERP * limot->m_currentLimitError * (rotational ? -1 : 1);
		if (rotational)
		{
			if (info->m_constraintError[srow] - vel * limot->m_stopERP > 0)
			{
				btScalar bounceerror = -limot->m_bounce * vel;
				if (bounceerror > info->m_constraintError[srow]) info->m_constraintError[srow] = bounceerror;
			}
		}
		else
		{
			if (info->m_constraintError[srow] - vel * limot->m_stopERP < 0)
			{
				btScalar bounceerror = -limot->m_bounce * vel;
				if (bounceerror < info->m_constraintError[srow]) info->m_constraintError[srow] = bounceerror;
			}
		}
		info->m_lowerLimit[srow] = rotational ? 0 : -SIMD_INFINITY;
		info->m_upperLimit[srow] = rotational ? SIMD_INFINITY : 0;
		info->cfm[srow] = limot->m_stopCFM;
		srow += info->rowskip;
		++count;

		calculateJacobi(limot, transA, transB, info, srow, ax1, rotational, rotAllowed);
		info->m_constraintError[srow] = info->fps * limot->m_stopERP * limot->m_currentLimitErrorHi * (rotational ? -1 : 1);
		if (rotational)
		{
			if (info->m_constraintError[srow] - vel * limot->m_stopERP < 0)
			{
				btScalar bounceerror = -limot->m_bounce * vel;
				if (bounceerror < info->m_constraintError[srow]) info->m_constraintError[srow] = bounceerror;
			}
		}
		else
		{
			if (info->m_constraintError[srow] - vel * limot->m_stopERP > 0)
			{
				btScalar bounceerror = -limot->m_bounce * vel;
				if (bounceerror > info->m_constraintError[srow]) info->m_constraintError[srow] = bounceerror;
			}
		}
		info->m_lowerLimit[srow] = rotational ? -SIMD_INFINITY : 0;
		info->m_upperLimit[srow] = rotational ? 0 : SIMD_INFINITY;
		info->cfm[srow] = limot->m_stopCFM;
		srow += info->rowskip;
		++count;
	}
	else if (limot->m_currentLimit == 3)
	{
		calculateJacobi(limot, transA, transB, info, srow, ax1, rotational, rotAllowed);
		info->m_constraintError[srow] = info->fps * limot->m_stopERP * limot->m_currentLimitError * (rotational ? -1 : 1);
		info->m_lowerLimit[srow] = -SIMD_INFINITY;
		info->m_upperLimit[srow] = SIMD_INFINITY;
		info->cfm[srow] = limot->m_stopCFM;
		srow += info->rowskip;
		++count;
	}

	if (limot->m_enableMotor && !limot->m_servoMotor)
	{
		calculateJacobi(limot, transA, transB, info, srow, ax1, rotational, rotAllowed);
		btScalar tag_vel = rotational ? limot->m_targetVelocity : -limot->m_targetVelocity;
		btScalar mot_fact = getMotorFactor(limot->m_currentPosition,
										   limot->m_loLimit,
										   limot->m_hiLimit,
										   tag_vel,
										   info->fps * limot->m_motorERP);
		info->m_constraintError[srow] = mot_fact * limot->m_targetVelocity;
		info->m_lowerLimit[srow] = -limot->m_maxMotorForce / info->fps;
		info->m_upperLimit[srow] = limot->m_maxMotorForce / info->fps;
		info->cfm[srow] = limot->m_motorCFM;
		srow += info->rowskip;
		++count;
	}

	if (limot->m_enableMotor && limot->m_servoMotor)
	{
		btScalar error = limot->m_currentPosition - limot->m_servoTarget;
		btScalar curServoTarget = limot->m_servoTarget;
		if (rotational)
		{
			if (error > SIMD_PI)
			{
				error -= SIMD_2_PI;
				curServoTarget += SIMD_2_PI;
			}
			if (error < -SIMD_PI)
			{
				error += SIMD_2_PI;
				curServoTarget -= SIMD_2_PI;
			}
		}

		calculateJacobi(limot, transA, transB, info, srow, ax1, rotational, rotAllowed);
		btScalar targetvelocity = error < 0 ? -limot->m_targetVelocity : limot->m_targetVelocity;
		btScalar tag_vel = -targetvelocity;
		btScalar mot_fact;
		if (error != 0)
		{
			btScalar lowLimit;
			btScalar hiLimit;
			if (limot->m_loLimit > limot->m_hiLimit)
			{
				lowLimit = error > 0 ? curServoTarget : -SIMD_INFINITY;
				hiLimit = error < 0 ? curServoTarget : SIMD_INFINITY;
			}
			else
			{
				lowLimit = error > 0 && curServoTarget > limot->m_loLimit ? curServoTarget : limot->m_loLimit;
				hiLimit = error < 0 && curServoTarget < limot->m_hiLimit ? curServoTarget : limot->m_hiLimit;
			}
			mot_fact = getMotorFactor(limot->m_currentPosition, lowLimit, hiLimit, tag_vel, info->fps * limot->m_motorERP);
		}
		else
		{
			mot_fact = 0;
		}
		info->m_constraintError[srow] = mot_fact * targetvelocity * (rotational ? -1 : 1);
		info->m_lowerLimit[srow] = -limot->m_maxMotorForce / info->fps;
		info->m_upperLimit[srow] = limot->m_maxMotorForce / info->fps;
		info->cfm[srow] = limot->m_motorCFM;
		srow += info->rowskip;
		++count;
	}

	if (limot->m_enableSpring)
	{
		btScalar error = limot->m_currentPosition - limot->m_equilibriumPoint;
		calculateJacobi(limot, transA, transB, info, srow, ax1, rotational, rotAllowed);

		
		
		
		
		
		
		

		btScalar dt = BT_ONE / info->fps;
		btScalar kd = limot->m_springDamping;
		btScalar ks = limot->m_springStiffness;
		btScalar vel;
		if (rotational)
		{
			vel = angVelA.dot(ax1) - angVelB.dot(ax1);
		}
		else
		{
			btVector3 tanVelA = angVelA.cross(m_calculatedTransformA.getOrigin() - transA.getOrigin());
			btVector3 tanVelB = angVelB.cross(m_calculatedTransformB.getOrigin() - transB.getOrigin());
			vel = (linVelA + tanVelA).dot(ax1) - (linVelB + tanVelB).dot(ax1);
		}
		btScalar cfm = BT_ZERO;
		btScalar mA = BT_ONE / m_rbA.getInvMass();
		btScalar mB = BT_ONE / m_rbB.getInvMass();
		if (rotational)
		{
			btScalar rrA = (m_calculatedTransformA.getOrigin() - transA.getOrigin()).length2();
			btScalar rrB = (m_calculatedTransformB.getOrigin() - transB.getOrigin()).length2();
			if (m_rbA.getInvMass()) mA = mA * rrA + 1 / (m_rbA.getInvInertiaTensorWorld() * ax1).length();
			if (m_rbB.getInvMass()) mB = mB * rrB + 1 / (m_rbB.getInvInertiaTensorWorld() * ax1).length();
		}
		btScalar m;
		if (m_rbA.getInvMass() == 0) m = mB; else
		if (m_rbB.getInvMass() == 0) m = mA; else
			m = mA*mB / (mA + mB);
		btScalar angularfreq = btSqrt(ks / m);

		
		if (limot->m_springStiffnessLimited && 0.25 < angularfreq * dt)
		{
			ks = BT_ONE / dt / dt / btScalar(16.0) * m;
		}
		
		if (limot->m_springDampingLimited && kd * dt > m)
		{
			kd = m / dt;
		}
		btScalar fs = ks * error * dt;
		btScalar fd = -kd * (vel) * (rotational ? -1 : 1) * dt;
		btScalar f = (fs + fd);

		
		
		
		
		
		
		
		
		
		
		
		
		
		
		if (m_flags & BT_6DOF_FLAGS_USE_INFINITE_ERROR)
			info->m_constraintError[srow] = (rotational ? -1 : 1) * (f < 0 ? -SIMD_INFINITY : SIMD_INFINITY);
		else
			info->m_constraintError[srow] = vel + f / m * (rotational ? -1 : 1);

		btScalar minf = f < fd ? f : fd;
		btScalar maxf = f < fd ? fd : f;
		if (!rotational)
		{
			info->m_lowerLimit[srow] = minf > 0 ? 0 : minf;
			info->m_upperLimit[srow] = maxf < 0 ? 0 : maxf;
		}
		else
		{
			info->m_lowerLimit[srow] = -maxf > 0 ? 0 : -maxf;
			info->m_upperLimit[srow] = -minf < 0 ? 0 : -minf;
		}

		info->cfm[srow] = cfm;
		srow += info->rowskip;
		++count;
	}

	return count;
}



void btGeneric6DofSpring2Constraint::setParam(int num, btScalar value, int axis)
{
	if ((axis >= 0) && (axis < 3))
	{
		switch (num)
		{
			case BT_CONSTRAINT_STOP_ERP:
				m_linearLimits.m_stopERP[axis] = value;
				m_flags |= BT_6DOF_FLAGS_ERP_STOP2 << (axis * BT_6DOF_FLAGS_AXIS_SHIFT2);
				break;
			case BT_CONSTRAINT_STOP_CFM:
				m_linearLimits.m_stopCFM[axis] = value;
				m_flags |= BT_6DOF_FLAGS_CFM_STOP2 << (axis * BT_6DOF_FLAGS_AXIS_SHIFT2);
				break;
			case BT_CONSTRAINT_ERP:
				m_linearLimits.m_motorERP[axis] = value;
				m_flags |= BT_6DOF_FLAGS_ERP_MOTO2 << (axis * BT_6DOF_FLAGS_AXIS_SHIFT2);
				break;
			case BT_CONSTRAINT_CFM:
				m_linearLimits.m_motorCFM[axis] = value;
				m_flags |= BT_6DOF_FLAGS_CFM_MOTO2 << (axis * BT_6DOF_FLAGS_AXIS_SHIFT2);
				break;
			default:
				btAssertConstrParams(0);
		}
	}
	else if ((axis >= 3) && (axis < 6))
	{
		switch (num)
		{
			case BT_CONSTRAINT_STOP_ERP:
				m_angularLimits[axis - 3].m_stopERP = value;
				m_flags |= BT_6DOF_FLAGS_ERP_STOP2 << (axis * BT_6DOF_FLAGS_AXIS_SHIFT2);
				break;
			case BT_CONSTRAINT_STOP_CFM:
				m_angularLimits[axis - 3].m_stopCFM = value;
				m_flags |= BT_6DOF_FLAGS_CFM_STOP2 << (axis * BT_6DOF_FLAGS_AXIS_SHIFT2);
				break;
			case BT_CONSTRAINT_ERP:
				m_angularLimits[axis - 3].m_motorERP = value;
				m_flags |= BT_6DOF_FLAGS_ERP_MOTO2 << (axis * BT_6DOF_FLAGS_AXIS_SHIFT2);
				break;
			case BT_CONSTRAINT_CFM:
				m_angularLimits[axis - 3].m_motorCFM = value;
				m_flags |= BT_6DOF_FLAGS_CFM_MOTO2 << (axis * BT_6DOF_FLAGS_AXIS_SHIFT2);
				break;
			default:
				btAssertConstrParams(0);
		}
	}
	else
	{
		btAssertConstrParams(0);
	}
}


btScalar btGeneric6DofSpring2Constraint::getParam(int num, int axis) const
{
	btScalar retVal = 0;
	if ((axis >= 0) && (axis < 3))
	{
		switch (num)
		{
			case BT_CONSTRAINT_STOP_ERP:
				btAssertConstrParams(m_flags & (BT_6DOF_FLAGS_ERP_STOP2 << (axis * BT_6DOF_FLAGS_AXIS_SHIFT2)));
				retVal = m_linearLimits.m_stopERP[axis];
				break;
			case BT_CONSTRAINT_STOP_CFM:
				btAssertConstrParams(m_flags & (BT_6DOF_FLAGS_CFM_STOP2 << (axis * BT_6DOF_FLAGS_AXIS_SHIFT2)));
				retVal = m_linearLimits.m_stopCFM[axis];
				break;
			case BT_CONSTRAINT_ERP:
				btAssertConstrParams(m_flags & (BT_6DOF_FLAGS_ERP_MOTO2 << (axis * BT_6DOF_FLAGS_AXIS_SHIFT2)));
				retVal = m_linearLimits.m_motorERP[axis];
				break;
			case BT_CONSTRAINT_CFM:
				btAssertConstrParams(m_flags & (BT_6DOF_FLAGS_CFM_MOTO2 << (axis * BT_6DOF_FLAGS_AXIS_SHIFT2)));
				retVal = m_linearLimits.m_motorCFM[axis];
				break;
			default:
				btAssertConstrParams(0);
		}
	}
	else if ((axis >= 3) && (axis < 6))
	{
		switch (num)
		{
			case BT_CONSTRAINT_STOP_ERP:
				btAssertConstrParams(m_flags & (BT_6DOF_FLAGS_ERP_STOP2 << (axis * BT_6DOF_FLAGS_AXIS_SHIFT2)));
				retVal = m_angularLimits[axis - 3].m_stopERP;
				break;
			case BT_CONSTRAINT_STOP_CFM:
				btAssertConstrParams(m_flags & (BT_6DOF_FLAGS_CFM_STOP2 << (axis * BT_6DOF_FLAGS_AXIS_SHIFT2)));
				retVal = m_angularLimits[axis - 3].m_stopCFM;
				break;
			case BT_CONSTRAINT_ERP:
				btAssertConstrParams(m_flags & (BT_6DOF_FLAGS_ERP_MOTO2 << (axis * BT_6DOF_FLAGS_AXIS_SHIFT2)));
				retVal = m_angularLimits[axis - 3].m_motorERP;
				break;
			case BT_CONSTRAINT_CFM:
				btAssertConstrParams(m_flags & (BT_6DOF_FLAGS_CFM_MOTO2 << (axis * BT_6DOF_FLAGS_AXIS_SHIFT2)));
				retVal = m_angularLimits[axis - 3].m_motorCFM;
				break;
			default:
				btAssertConstrParams(0);
		}
	}
	else
	{
		btAssertConstrParams(0);
	}
	return retVal;
}

void btGeneric6DofSpring2Constraint::setAxis(const btVector3& axis1, const btVector3& axis2)
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

void btGeneric6DofSpring2Constraint::setBounce(int index, btScalar bounce)
{
	btAssert((index >= 0) && (index < 6));
	if (index < 3)
		m_linearLimits.m_bounce[index] = bounce;
	else
		m_angularLimits[index - 3].m_bounce = bounce;
}

void btGeneric6DofSpring2Constraint::enableMotor(int index, bool onOff)
{
	btAssert((index >= 0) && (index < 6));
	if (index < 3)
		m_linearLimits.m_enableMotor[index] = onOff;
	else
		m_angularLimits[index - 3].m_enableMotor = onOff;
}

void btGeneric6DofSpring2Constraint::setServo(int index, bool onOff)
{
	btAssert((index >= 0) && (index < 6));
	if (index < 3)
		m_linearLimits.m_servoMotor[index] = onOff;
	else
		m_angularLimits[index - 3].m_servoMotor = onOff;
}

void btGeneric6DofSpring2Constraint::setTargetVelocity(int index, btScalar velocity)
{
	btAssert((index >= 0) && (index < 6));
	if (index < 3)
		m_linearLimits.m_targetVelocity[index] = velocity;
	else
		m_angularLimits[index - 3].m_targetVelocity = velocity;
}

void btGeneric6DofSpring2Constraint::setServoTarget(int index, btScalar targetOrg)
{
	btAssert((index >= 0) && (index < 6));
	if (index < 3)
	{
		m_linearLimits.m_servoTarget[index] = targetOrg;
	}
	else
	{
		
		

		btScalar target = targetOrg + SIMD_PI;
		if (1)
		{
			btScalar m = target - SIMD_2_PI * std::floor(target / SIMD_2_PI);
			
			{
				if (m >= SIMD_2_PI)
				{
					target = 0;
				}
				else
				{
					if (m < 0)
					{
						if (SIMD_2_PI + m == SIMD_2_PI)
							target = 0;
						else
							target = SIMD_2_PI + m;
					}
					else
					{
						target = m;
					}
				}
			}
			target -= SIMD_PI;
		}

		m_angularLimits[index - 3].m_servoTarget = target;
	}
}

void btGeneric6DofSpring2Constraint::setMaxMotorForce(int index, btScalar force)
{
	btAssert((index >= 0) && (index < 6));
	if (index < 3)
		m_linearLimits.m_maxMotorForce[index] = force;
	else
		m_angularLimits[index - 3].m_maxMotorForce = force;
}

void btGeneric6DofSpring2Constraint::enableSpring(int index, bool onOff)
{
	btAssert((index >= 0) && (index < 6));
	if (index < 3)
		m_linearLimits.m_enableSpring[index] = onOff;
	else
		m_angularLimits[index - 3].m_enableSpring = onOff;
}

void btGeneric6DofSpring2Constraint::setStiffness(int index, btScalar stiffness, bool limitIfNeeded)
{
	btAssert((index >= 0) && (index < 6));
	if (index < 3)
	{
		m_linearLimits.m_springStiffness[index] = stiffness;
		m_linearLimits.m_springStiffnessLimited[index] = limitIfNeeded;
	}
	else
	{
		m_angularLimits[index - 3].m_springStiffness = stiffness;
		m_angularLimits[index - 3].m_springStiffnessLimited = limitIfNeeded;
	}
}

void btGeneric6DofSpring2Constraint::setDamping(int index, btScalar damping, bool limitIfNeeded)
{
	btAssert((index >= 0) && (index < 6));
	if (index < 3)
	{
		m_linearLimits.m_springDamping[index] = damping;
		m_linearLimits.m_springDampingLimited[index] = limitIfNeeded;
	}
	else
	{
		m_angularLimits[index - 3].m_springDamping = damping;
		m_angularLimits[index - 3].m_springDampingLimited = limitIfNeeded;
	}
}

void btGeneric6DofSpring2Constraint::setEquilibriumPoint()
{
	calculateTransforms();
	int i;
	for (i = 0; i < 3; i++)
		m_linearLimits.m_equilibriumPoint[i] = m_calculatedLinearDiff[i];
	for (i = 0; i < 3; i++)
		m_angularLimits[i].m_equilibriumPoint = m_calculatedAxisAngleDiff[i];
}

void btGeneric6DofSpring2Constraint::setEquilibriumPoint(int index)
{
	btAssert((index >= 0) && (index < 6));
	calculateTransforms();
	if (index < 3)
		m_linearLimits.m_equilibriumPoint[index] = m_calculatedLinearDiff[index];
	else
		m_angularLimits[index - 3].m_equilibriumPoint = m_calculatedAxisAngleDiff[index - 3];
}

void btGeneric6DofSpring2Constraint::setEquilibriumPoint(int index, btScalar val)
{
	btAssert((index >= 0) && (index < 6));
	if (index < 3)
		m_linearLimits.m_equilibriumPoint[index] = val;
	else
		m_angularLimits[index - 3].m_equilibriumPoint = val;
}



void btRotationalLimitMotor2::testLimitValue(btScalar test_value)
{
	
	if (m_loLimit > m_hiLimit)
	{
		m_currentLimit = 0;
		m_currentLimitError = btScalar(0.f);
	}
	else if (m_loLimit == m_hiLimit)
	{
		m_currentLimitError = test_value - m_loLimit;
		m_currentLimit = 3;
	}
	else
	{
		m_currentLimitError = test_value - m_loLimit;
		m_currentLimitErrorHi = test_value - m_hiLimit;
		m_currentLimit = 4;
	}
}



void btTranslationalLimitMotor2::testLimitValue(int limitIndex, btScalar test_value)
{
	btScalar loLimit = m_lowerLimit[limitIndex];
	btScalar hiLimit = m_upperLimit[limitIndex];
	if (loLimit > hiLimit)
	{
		m_currentLimitError[limitIndex] = 0;
		m_currentLimit[limitIndex] = 0;
	}
	else if (loLimit == hiLimit)
	{
		m_currentLimitError[limitIndex] = test_value - loLimit;
		m_currentLimit[limitIndex] = 3;
	}
	else
	{
		m_currentLimitError[limitIndex] = test_value - loLimit;
		m_currentLimitErrorHi[limitIndex] = test_value - hiLimit;
		m_currentLimit[limitIndex] = 4;
	}
}
