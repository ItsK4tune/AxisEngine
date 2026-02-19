

#include "btRigidBody.h"
#include "BulletCollision/CollisionShapes/btConvexShape.h"
#include "LinearMath/btMinMax.h"
#include "LinearMath/btTransformUtil.h"
#include "LinearMath/btMotionState.h"
#include "BulletDynamics/ConstraintSolver/btTypedConstraint.h"
#include "LinearMath/btSerializer.h"


btScalar gDeactivationTime = btScalar(2.);
bool gDisableDeactivation = false;
static int uniqueId = 0;

btRigidBody::btRigidBody(const btRigidBody::btRigidBodyConstructionInfo& constructionInfo)
{
	setupRigidBody(constructionInfo);
}

btRigidBody::btRigidBody(btScalar mass, btMotionState* motionState, btCollisionShape* collisionShape, const btVector3& localInertia)
{
	btRigidBodyConstructionInfo cinfo(mass, motionState, collisionShape, localInertia);
	setupRigidBody(cinfo);
}

void btRigidBody::setupRigidBody(const btRigidBody::btRigidBodyConstructionInfo& constructionInfo)
{
	m_internalType = CO_RIGID_BODY;

	m_linearVelocity.setValue(btScalar(0.0), btScalar(0.0), btScalar(0.0));
	m_angularVelocity.setValue(btScalar(0.), btScalar(0.), btScalar(0.));
	m_angularFactor.setValue(1, 1, 1);
	m_linearFactor.setValue(1, 1, 1);
	m_gravity.setValue(btScalar(0.0), btScalar(0.0), btScalar(0.0));
	m_gravity_acceleration.setValue(btScalar(0.0), btScalar(0.0), btScalar(0.0));
	m_totalForce.setValue(btScalar(0.0), btScalar(0.0), btScalar(0.0));
	m_totalTorque.setValue(btScalar(0.0), btScalar(0.0), btScalar(0.0)),
		setDamping(constructionInfo.m_linearDamping, constructionInfo.m_angularDamping);

	m_linearSleepingThreshold = constructionInfo.m_linearSleepingThreshold;
	m_angularSleepingThreshold = constructionInfo.m_angularSleepingThreshold;
	m_optionalMotionState = constructionInfo.m_motionState;
	m_contactSolverType = 0;
	m_frictionSolverType = 0;
	m_additionalDamping = constructionInfo.m_additionalDamping;
	m_additionalDampingFactor = constructionInfo.m_additionalDampingFactor;
	m_additionalLinearDampingThresholdSqr = constructionInfo.m_additionalLinearDampingThresholdSqr;
	m_additionalAngularDampingThresholdSqr = constructionInfo.m_additionalAngularDampingThresholdSqr;
	m_additionalAngularDampingFactor = constructionInfo.m_additionalAngularDampingFactor;

	if (m_optionalMotionState)
	{
		m_optionalMotionState->getWorldTransform(m_worldTransform);
	}
	else
	{
		m_worldTransform = constructionInfo.m_startWorldTransform;
	}

	m_interpolationWorldTransform = m_worldTransform;
	m_interpolationLinearVelocity.setValue(0, 0, 0);
	m_interpolationAngularVelocity.setValue(0, 0, 0);

	
	m_friction = constructionInfo.m_friction;
	m_rollingFriction = constructionInfo.m_rollingFriction;
	m_spinningFriction = constructionInfo.m_spinningFriction;

	m_restitution = constructionInfo.m_restitution;

	setCollisionShape(constructionInfo.m_collisionShape);
	m_debugBodyId = uniqueId++;

	setMassProps(constructionInfo.m_mass, constructionInfo.m_localInertia);
	updateInertiaTensor();

	m_rigidbodyFlags = BT_ENABLE_GYROSCOPIC_FORCE_IMPLICIT_BODY;

	m_deltaLinearVelocity.setZero();
	m_deltaAngularVelocity.setZero();
	m_invMass = m_inverseMass * m_linearFactor;
	m_pushVelocity.setZero();
	m_turnVelocity.setZero();
}

void btRigidBody::predictIntegratedTransform(btScalar timeStep, btTransform& predictedTransform)
{
	btTransformUtil::integrateTransform(m_worldTransform, m_linearVelocity, m_angularVelocity, timeStep, predictedTransform);
}

void btRigidBody::saveKinematicState(btScalar timeStep)
{
	
	if (timeStep != btScalar(0.))
	{
		
		if (getMotionState())
			getMotionState()->getWorldTransform(m_worldTransform);
		btVector3 linVel, angVel;

		btTransformUtil::calculateVelocity(m_interpolationWorldTransform, m_worldTransform, timeStep, m_linearVelocity, m_angularVelocity);
		m_interpolationLinearVelocity = m_linearVelocity;
		m_interpolationAngularVelocity = m_angularVelocity;
		m_interpolationWorldTransform = m_worldTransform;
		
	}
}

void btRigidBody::getAabb(btVector3& aabbMin, btVector3& aabbMax) const
{
	getCollisionShape()->getAabb(m_worldTransform, aabbMin, aabbMax);
}

void btRigidBody::setGravity(const btVector3& acceleration)
{
	if (m_inverseMass != btScalar(0.0))
	{
		m_gravity = acceleration * (btScalar(1.0) / m_inverseMass);
	}
	m_gravity_acceleration = acceleration;
}

void btRigidBody::setDamping(btScalar lin_damping, btScalar ang_damping)
{
#ifdef BT_USE_OLD_DAMPING_METHOD
	m_linearDamping = btMax(lin_damping, btScalar(0.0));
	m_angularDamping = btMax(ang_damping, btScalar(0.0));
#else
	m_linearDamping = btClamped(lin_damping, btScalar(0.0), btScalar(1.0));
	m_angularDamping = btClamped(ang_damping, btScalar(0.0), btScalar(1.0));
#endif
}


void btRigidBody::applyDamping(btScalar timeStep)
{
	
	

#ifdef BT_USE_OLD_DAMPING_METHOD
	m_linearVelocity *= btMax((btScalar(1.0) - timeStep * m_linearDamping), btScalar(0.0));
	m_angularVelocity *= btMax((btScalar(1.0) - timeStep * m_angularDamping), btScalar(0.0));
#else
	m_linearVelocity *= btPow(btScalar(1) - m_linearDamping, timeStep);
	m_angularVelocity *= btPow(btScalar(1) - m_angularDamping, timeStep);
#endif

	if (m_additionalDamping)
	{
		
		
		if ((m_angularVelocity.length2() < m_additionalAngularDampingThresholdSqr) &&
			(m_linearVelocity.length2() < m_additionalLinearDampingThresholdSqr))
		{
			m_angularVelocity *= m_additionalDampingFactor;
			m_linearVelocity *= m_additionalDampingFactor;
		}

		btScalar speed = m_linearVelocity.length();
		if (speed < m_linearDamping)
		{
			btScalar dampVel = btScalar(0.005);
			if (speed > dampVel)
			{
				btVector3 dir = m_linearVelocity.normalized();
				m_linearVelocity -= dir * dampVel;
			}
			else
			{
				m_linearVelocity.setValue(btScalar(0.), btScalar(0.), btScalar(0.));
			}
		}

		btScalar angSpeed = m_angularVelocity.length();
		if (angSpeed < m_angularDamping)
		{
			btScalar angDampVel = btScalar(0.005);
			if (angSpeed > angDampVel)
			{
				btVector3 dir = m_angularVelocity.normalized();
				m_angularVelocity -= dir * angDampVel;
			}
			else
			{
				m_angularVelocity.setValue(btScalar(0.), btScalar(0.), btScalar(0.));
			}
		}
	}
}

void btRigidBody::applyGravity()
{
	if (isStaticOrKinematicObject())
		return;

	applyCentralForce(m_gravity);
}

void btRigidBody::clearGravity()
{
    if (isStaticOrKinematicObject())
        return;
    
    applyCentralForce(-m_gravity);
}

void btRigidBody::proceedToTransform(const btTransform& newTrans)
{
	setCenterOfMassTransform(newTrans);
}

void btRigidBody::setMassProps(btScalar mass, const btVector3& inertia)
{
	if (mass == btScalar(0.))
	{
		m_collisionFlags |= btCollisionObject::CF_STATIC_OBJECT;
		m_inverseMass = btScalar(0.);
	}
	else
	{
		m_collisionFlags &= (~btCollisionObject::CF_STATIC_OBJECT);
		m_inverseMass = btScalar(1.0) / mass;
	}

	
	m_gravity = mass * m_gravity_acceleration;

	m_invInertiaLocal.setValue(inertia.x() != btScalar(0.0) ? btScalar(1.0) / inertia.x() : btScalar(0.0),
							   inertia.y() != btScalar(0.0) ? btScalar(1.0) / inertia.y() : btScalar(0.0),
							   inertia.z() != btScalar(0.0) ? btScalar(1.0) / inertia.z() : btScalar(0.0));

	m_invMass = m_linearFactor * m_inverseMass;
}

void btRigidBody::updateInertiaTensor()
{
	m_invInertiaTensorWorld = m_worldTransform.getBasis().scaled(m_invInertiaLocal) * m_worldTransform.getBasis().transpose();
}

btVector3 btRigidBody::getLocalInertia() const
{
	btVector3 inertiaLocal;
	const btVector3 inertia = m_invInertiaLocal;
	inertiaLocal.setValue(inertia.x() != btScalar(0.0) ? btScalar(1.0) / inertia.x() : btScalar(0.0),
						  inertia.y() != btScalar(0.0) ? btScalar(1.0) / inertia.y() : btScalar(0.0),
						  inertia.z() != btScalar(0.0) ? btScalar(1.0) / inertia.z() : btScalar(0.0));
	return inertiaLocal;
}

inline btVector3 evalEulerEqn(const btVector3& w1, const btVector3& w0, const btVector3& T, const btScalar dt,
							  const btMatrix3x3& I)
{
	const btVector3 w2 = I * w1 + w1.cross(I * w1) * dt - (T * dt + I * w0);
	return w2;
}

inline btMatrix3x3 evalEulerEqnDeriv(const btVector3& w1, const btVector3& w0, const btScalar dt,
									 const btMatrix3x3& I)
{
	btMatrix3x3 w1x, Iw1x;
	const btVector3 Iwi = (I * w1);
	w1.getSkewSymmetricMatrix(&w1x[0], &w1x[1], &w1x[2]);
	Iwi.getSkewSymmetricMatrix(&Iw1x[0], &Iw1x[1], &Iw1x[2]);

	const btMatrix3x3 dfw1 = I + (w1x * I - Iw1x) * dt;
	return dfw1;
}

btVector3 btRigidBody::computeGyroscopicForceExplicit(btScalar maxGyroscopicForce) const
{
	btVector3 inertiaLocal = getLocalInertia();
	btMatrix3x3 inertiaTensorWorld = getWorldTransform().getBasis().scaled(inertiaLocal) * getWorldTransform().getBasis().transpose();
	btVector3 tmp = inertiaTensorWorld * getAngularVelocity();
	btVector3 gf = getAngularVelocity().cross(tmp);
	btScalar l2 = gf.length2();
	if (l2 > maxGyroscopicForce * maxGyroscopicForce)
	{
		gf *= btScalar(1.) / btSqrt(l2) * maxGyroscopicForce;
	}
	return gf;
}

btVector3 btRigidBody::computeGyroscopicImpulseImplicit_Body(btScalar step) const
{
	btVector3 idl = getLocalInertia();
	btVector3 omega1 = getAngularVelocity();
	btQuaternion q = getWorldTransform().getRotation();

	
	btVector3 omegab = quatRotate(q.inverse(), omega1);
	btMatrix3x3 Ib;
	Ib.setValue(idl.x(), 0, 0,
				0, idl.y(), 0,
				0, 0, idl.z());

	btVector3 ibo = Ib * omegab;

	
	btVector3 f = step * omegab.cross(ibo);

	btMatrix3x3 skew0;
	omegab.getSkewSymmetricMatrix(&skew0[0], &skew0[1], &skew0[2]);
	btVector3 om = Ib * omegab;
	btMatrix3x3 skew1;
	om.getSkewSymmetricMatrix(&skew1[0], &skew1[1], &skew1[2]);

	
	btMatrix3x3 J = Ib + (skew0 * Ib - skew1) * step;

	
	
	btVector3 omega_div = J.solve33(f);

	
	omegab = omegab - omega_div;  
	
	btVector3 omega2 = quatRotate(q, omegab);
	btVector3 gf = omega2 - omega1;
	return gf;
}

btVector3 btRigidBody::computeGyroscopicImpulseImplicit_World(btScalar step) const
{
	
	

	const btVector3 inertiaLocal = getLocalInertia();
	const btVector3 w0 = getAngularVelocity();

	btMatrix3x3 I;

	I = m_worldTransform.getBasis().scaled(inertiaLocal) *
		m_worldTransform.getBasis().transpose();

	
	
	

	btVector3 w1 = w0;

	
	{
		const btVector3 fw = evalEulerEqn(w1, w0, btVector3(0, 0, 0), step, I);
		const btMatrix3x3 dfw = evalEulerEqnDeriv(w1, w0, step, I);

		btVector3 dw;
		dw = dfw.solve33(fw);
		
		

		w1 -= dw;
	}

	btVector3 gf = (w1 - w0);
	return gf;
}

void btRigidBody::integrateVelocities(btScalar step)
{
	if (isStaticOrKinematicObject())
		return;

	m_linearVelocity += m_totalForce * (m_inverseMass * step);
	m_angularVelocity += m_invInertiaTensorWorld * m_totalTorque * step;

#define MAX_ANGVEL SIMD_HALF_PI
	
	btScalar angvel = m_angularVelocity.length();
	if (angvel * step > MAX_ANGVEL)
	{
		m_angularVelocity *= (MAX_ANGVEL / step) / angvel;
	}
	#if defined(BT_CLAMP_VELOCITY_TO) && BT_CLAMP_VELOCITY_TO > 0
	clampVelocity(m_angularVelocity);
	#endif
}

btQuaternion btRigidBody::getOrientation() const
{
	btQuaternion orn;
	m_worldTransform.getBasis().getRotation(orn);
	return orn;
}

void btRigidBody::setCenterOfMassTransform(const btTransform& xform)
{
	if (isKinematicObject())
	{
		m_interpolationWorldTransform = m_worldTransform;
	}
	else
	{
		m_interpolationWorldTransform = xform;
	}
	m_interpolationLinearVelocity = getLinearVelocity();
	m_interpolationAngularVelocity = getAngularVelocity();
	m_worldTransform = xform;
	updateInertiaTensor();
}

void btRigidBody::addConstraintRef(btTypedConstraint* c)
{
	

	int index = m_constraintRefs.findLinearSearch(c);
	
	
	if (index == m_constraintRefs.size())
	{
		m_constraintRefs.push_back(c);
		btCollisionObject* colObjA = &c->getRigidBodyA();
		btCollisionObject* colObjB = &c->getRigidBodyB();
		if (colObjA == this)
		{
			colObjA->setIgnoreCollisionCheck(colObjB, true);
		}
		else
		{
			colObjB->setIgnoreCollisionCheck(colObjA, true);
		}
	}
}

void btRigidBody::removeConstraintRef(btTypedConstraint* c)
{
	int index = m_constraintRefs.findLinearSearch(c);
	
	if (index < m_constraintRefs.size())
	{
		m_constraintRefs.remove(c);
		btCollisionObject* colObjA = &c->getRigidBodyA();
		btCollisionObject* colObjB = &c->getRigidBodyB();
		if (colObjA == this)
		{
			colObjA->setIgnoreCollisionCheck(colObjB, false);
		}
		else
		{
			colObjB->setIgnoreCollisionCheck(colObjA, false);
		}
	}
}

int btRigidBody::calculateSerializeBufferSize() const
{
	int sz = sizeof(btRigidBodyData);
	return sz;
}


const char* btRigidBody::serialize(void* dataBuffer, class btSerializer* serializer) const
{
	btRigidBodyData* rbd = (btRigidBodyData*)dataBuffer;

	btCollisionObject::serialize(&rbd->m_collisionObjectData, serializer);

	m_invInertiaTensorWorld.serialize(rbd->m_invInertiaTensorWorld);
	m_linearVelocity.serialize(rbd->m_linearVelocity);
	m_angularVelocity.serialize(rbd->m_angularVelocity);
	rbd->m_inverseMass = m_inverseMass;
	m_angularFactor.serialize(rbd->m_angularFactor);
	m_linearFactor.serialize(rbd->m_linearFactor);
	m_gravity.serialize(rbd->m_gravity);
	m_gravity_acceleration.serialize(rbd->m_gravity_acceleration);
	m_invInertiaLocal.serialize(rbd->m_invInertiaLocal);
	m_totalForce.serialize(rbd->m_totalForce);
	m_totalTorque.serialize(rbd->m_totalTorque);
	rbd->m_linearDamping = m_linearDamping;
	rbd->m_angularDamping = m_angularDamping;
	rbd->m_additionalDamping = m_additionalDamping;
	rbd->m_additionalDampingFactor = m_additionalDampingFactor;
	rbd->m_additionalLinearDampingThresholdSqr = m_additionalLinearDampingThresholdSqr;
	rbd->m_additionalAngularDampingThresholdSqr = m_additionalAngularDampingThresholdSqr;
	rbd->m_additionalAngularDampingFactor = m_additionalAngularDampingFactor;
	rbd->m_linearSleepingThreshold = m_linearSleepingThreshold;
	rbd->m_angularSleepingThreshold = m_angularSleepingThreshold;

	
#ifdef BT_USE_DOUBLE_PRECISION
	memset(rbd->m_padding, 0, sizeof(rbd->m_padding));
#endif

	return btRigidBodyDataName;
}

void btRigidBody::serializeSingleObject(class btSerializer* serializer) const
{
	btChunk* chunk = serializer->allocate(calculateSerializeBufferSize(), 1);
	const char* structType = serialize(chunk->m_oldPtr, serializer);
	serializer->finalizeChunk(chunk, structType, BT_RIGIDBODY_CODE, (void*)this);
}
