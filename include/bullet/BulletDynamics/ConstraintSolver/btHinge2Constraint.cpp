

#include "btHinge2Constraint.h"
#include "BulletDynamics/Dynamics/btRigidBody.h"
#include "LinearMath/btTransformUtil.h"




btHinge2Constraint::btHinge2Constraint(btRigidBody& rbA, btRigidBody& rbB, btVector3& anchor, btVector3& axis1, btVector3& axis2)
	: btGeneric6DofSpring2Constraint(rbA, rbB, btTransform::getIdentity(), btTransform::getIdentity(), RO_XYZ),
	  m_anchor(anchor),
	  m_axis1(axis1),
	  m_axis2(axis2)
{
	
	
	
	
	
	
	
	
	
	btVector3 zAxis = axis1.normalize();
	btVector3 xAxis = axis2.normalize();
	btVector3 yAxis = zAxis.cross(xAxis);  
	btTransform frameInW;
	frameInW.setIdentity();
	frameInW.getBasis().setValue(xAxis[0], yAxis[0], zAxis[0],
								 xAxis[1], yAxis[1], zAxis[1],
								 xAxis[2], yAxis[2], zAxis[2]);
	frameInW.setOrigin(anchor);
	
	m_frameInA = rbA.getCenterOfMassTransform().inverse() * frameInW;
	m_frameInB = rbB.getCenterOfMassTransform().inverse() * frameInW;
	
	setLinearLowerLimit(btVector3(0.f, 0.f, -1.f));
	setLinearUpperLimit(btVector3(0.f, 0.f, 1.f));
	
	setAngularLowerLimit(btVector3(1.f, 0.f, -SIMD_HALF_PI * 0.5f));
	setAngularUpperLimit(btVector3(-1.f, 0.f, SIMD_HALF_PI * 0.5f));
	
	enableSpring(2, true);
	setStiffness(2, SIMD_PI * SIMD_PI * 4.f);
	setDamping(2, 0.01f);
	setEquilibriumPoint();
}
