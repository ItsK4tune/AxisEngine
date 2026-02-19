

#include "btUniversalConstraint.h"
#include "BulletDynamics/Dynamics/btRigidBody.h"
#include "LinearMath/btTransformUtil.h"

#define UNIV_EPS btScalar(0.01f)




btUniversalConstraint::btUniversalConstraint(btRigidBody& rbA, btRigidBody& rbB, const btVector3& anchor, const btVector3& axis1, const btVector3& axis2)
	: btGeneric6DofConstraint(rbA, rbB, btTransform::getIdentity(), btTransform::getIdentity(), true),
	  m_anchor(anchor),
	  m_axis1(axis1),
	  m_axis2(axis2)
{
	
	
	
	
	
	
	
	
	
	btVector3 zAxis = m_axis1.normalize();
	btVector3 yAxis = m_axis2.normalize();
	btVector3 xAxis = yAxis.cross(zAxis);  
	btTransform frameInW;
	frameInW.setIdentity();
	frameInW.getBasis().setValue(xAxis[0], yAxis[0], zAxis[0],
								 xAxis[1], yAxis[1], zAxis[1],
								 xAxis[2], yAxis[2], zAxis[2]);
	frameInW.setOrigin(anchor);
	
	m_frameInA = rbA.getCenterOfMassTransform().inverse() * frameInW;
	m_frameInB = rbB.getCenterOfMassTransform().inverse() * frameInW;
	
	setLinearLowerLimit(btVector3(0., 0., 0.));
	setLinearUpperLimit(btVector3(0., 0., 0.));
	setAngularLowerLimit(btVector3(0.f, -SIMD_HALF_PI + UNIV_EPS, -SIMD_PI + UNIV_EPS));
	setAngularUpperLimit(btVector3(0.f, SIMD_HALF_PI - UNIV_EPS, SIMD_PI - UNIV_EPS));
}

void btUniversalConstraint::setAxis(const btVector3& axis1, const btVector3& axis2)
{
	m_axis1 = axis1;
	m_axis2 = axis2;

	btVector3 zAxis = axis1.normalized();
	btVector3 yAxis = axis2.normalized();
	btVector3 xAxis = yAxis.cross(zAxis);  

	btTransform frameInW;
	frameInW.setIdentity();
	frameInW.getBasis().setValue(xAxis[0], yAxis[0], zAxis[0],
								 xAxis[1], yAxis[1], zAxis[1],
								 xAxis[2], yAxis[2], zAxis[2]);
	frameInW.setOrigin(m_anchor);

	
	m_frameInA = m_rbA.getCenterOfMassTransform().inverse() * frameInW;
	m_frameInB = m_rbB.getCenterOfMassTransform().inverse() * frameInW;

	calculateTransforms();
}
