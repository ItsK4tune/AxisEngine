

#ifndef BT_TRANSFORM_UTIL_H
#define BT_TRANSFORM_UTIL_H

#include "btTransform.h"
#define ANGULAR_MOTION_THRESHOLD btScalar(0.5) * SIMD_HALF_PI

SIMD_FORCE_INLINE btVector3 btAabbSupport(const btVector3& halfExtents, const btVector3& supportDir)
{
	return btVector3(supportDir.x() < btScalar(0.0) ? -halfExtents.x() : halfExtents.x(),
					 supportDir.y() < btScalar(0.0) ? -halfExtents.y() : halfExtents.y(),
					 supportDir.z() < btScalar(0.0) ? -halfExtents.z() : halfExtents.z());
}


class btTransformUtil
{
public:
	static void integrateTransform(const btTransform& curTrans, const btVector3& linvel, const btVector3& angvel, btScalar timeStep, btTransform& predictedTransform)
	{
		predictedTransform.setOrigin(curTrans.getOrigin() + linvel * timeStep);
		
#ifdef QUATERNION_DERIVATIVE
		btQuaternion predictedOrn = curTrans.getRotation();
		predictedOrn += (angvel * predictedOrn) * (timeStep * btScalar(0.5));
		predictedOrn.safeNormalize();
#else
		
		

		btVector3 axis;
		btScalar fAngle2 = angvel.length2();
		btScalar fAngle = 0;
		if (fAngle2 > SIMD_EPSILON)
		{
			fAngle = btSqrt(fAngle2);
		}

		
		if (fAngle * timeStep > ANGULAR_MOTION_THRESHOLD)
		{
			fAngle = ANGULAR_MOTION_THRESHOLD / timeStep;
		}

		if (fAngle < btScalar(0.001))
		{
			
			axis = angvel * (btScalar(0.5) * timeStep - (timeStep * timeStep * timeStep) * (btScalar(0.020833333333)) * fAngle * fAngle);
		}
		else
		{
			
			axis = angvel * (btSin(btScalar(0.5) * fAngle * timeStep) / fAngle);
		}
		btQuaternion dorn(axis.x(), axis.y(), axis.z(), btCos(fAngle * timeStep * btScalar(0.5)));
		btQuaternion orn0 = curTrans.getRotation();

		btQuaternion predictedOrn = dorn * orn0;
		predictedOrn.safeNormalize();
#endif
		if (predictedOrn.length2() > SIMD_EPSILON)
		{
			predictedTransform.setRotation(predictedOrn);
		}
		else
		{
			predictedTransform.setBasis(curTrans.getBasis());
		}
	}

	static void calculateVelocityQuaternion(const btVector3& pos0, const btVector3& pos1, const btQuaternion& orn0, const btQuaternion& orn1, btScalar timeStep, btVector3& linVel, btVector3& angVel)
	{
		linVel = (pos1 - pos0) / timeStep;
		btVector3 axis;
		btScalar angle;
		if (orn0 != orn1)
		{
			calculateDiffAxisAngleQuaternion(orn0, orn1, axis, angle);
			angVel = axis * angle / timeStep;
		}
		else
		{
			angVel.setValue(0, 0, 0);
		}
	}

	static void calculateDiffAxisAngleQuaternion(const btQuaternion& orn0, const btQuaternion& orn1a, btVector3& axis, btScalar& angle)
	{
		btQuaternion orn1 = orn0.nearest(orn1a);
		btQuaternion dorn = orn1 * orn0.inverse();
		angle = dorn.getAngle();
		axis = btVector3(dorn.x(), dorn.y(), dorn.z());
		axis[3] = btScalar(0.);
		
		btScalar len = axis.length2();
		if (len < SIMD_EPSILON * SIMD_EPSILON)
			axis = btVector3(btScalar(1.), btScalar(0.), btScalar(0.));
		else
			axis /= btSqrt(len);
	}

	static void calculateVelocity(const btTransform& transform0, const btTransform& transform1, btScalar timeStep, btVector3& linVel, btVector3& angVel)
	{
		linVel = (transform1.getOrigin() - transform0.getOrigin()) / timeStep;
		btVector3 axis;
		btScalar angle;
		calculateDiffAxisAngle(transform0, transform1, axis, angle);
		angVel = axis * angle / timeStep;
	}

	static void calculateDiffAxisAngle(const btTransform& transform0, const btTransform& transform1, btVector3& axis, btScalar& angle)
	{
		btMatrix3x3 dmat = transform1.getBasis() * transform0.getBasis().inverse();
		btQuaternion dorn;
		dmat.getRotation(dorn);

		
		dorn.normalize();

		angle = dorn.getAngle();
		axis = btVector3(dorn.x(), dorn.y(), dorn.z());
		axis[3] = btScalar(0.);
		
		btScalar len = axis.length2();
		if (len < SIMD_EPSILON * SIMD_EPSILON)
			axis = btVector3(btScalar(1.), btScalar(0.), btScalar(0.));
		else
			axis /= btSqrt(len);
	}
};



class btConvexSeparatingDistanceUtil
{
	btQuaternion m_ornA;
	btQuaternion m_ornB;
	btVector3 m_posA;
	btVector3 m_posB;

	btVector3 m_separatingNormal;

	btScalar m_boundingRadiusA;
	btScalar m_boundingRadiusB;
	btScalar m_separatingDistance;

public:
	btConvexSeparatingDistanceUtil(btScalar boundingRadiusA, btScalar boundingRadiusB)
		: m_boundingRadiusA(boundingRadiusA),
		  m_boundingRadiusB(boundingRadiusB),
		  m_separatingDistance(0.f)
	{
	}

	btScalar getConservativeSeparatingDistance()
	{
		return m_separatingDistance;
	}

	void updateSeparatingDistance(const btTransform& transA, const btTransform& transB)
	{
		const btVector3& toPosA = transA.getOrigin();
		const btVector3& toPosB = transB.getOrigin();
		btQuaternion toOrnA = transA.getRotation();
		btQuaternion toOrnB = transB.getRotation();

		if (m_separatingDistance > 0.f)
		{
			btVector3 linVelA, angVelA, linVelB, angVelB;
			btTransformUtil::calculateVelocityQuaternion(m_posA, toPosA, m_ornA, toOrnA, btScalar(1.), linVelA, angVelA);
			btTransformUtil::calculateVelocityQuaternion(m_posB, toPosB, m_ornB, toOrnB, btScalar(1.), linVelB, angVelB);
			btScalar maxAngularProjectedVelocity = angVelA.length() * m_boundingRadiusA + angVelB.length() * m_boundingRadiusB;
			btVector3 relLinVel = (linVelB - linVelA);
			btScalar relLinVelocLength = relLinVel.dot(m_separatingNormal);
			if (relLinVelocLength < 0.f)
			{
				relLinVelocLength = 0.f;
			}

			btScalar projectedMotion = maxAngularProjectedVelocity + relLinVelocLength;
			m_separatingDistance -= projectedMotion;
		}

		m_posA = toPosA;
		m_posB = toPosB;
		m_ornA = toOrnA;
		m_ornB = toOrnB;
	}

	void initSeparatingDistance(const btVector3& separatingVector, btScalar separatingDistance, const btTransform& transA, const btTransform& transB)
	{
		m_separatingDistance = separatingDistance;

		if (m_separatingDistance > 0.f)
		{
			m_separatingNormal = separatingVector;

			const btVector3& toPosA = transA.getOrigin();
			const btVector3& toPosB = transB.getOrigin();
			btQuaternion toOrnA = transA.getRotation();
			btQuaternion toOrnB = transB.getRotation();
			m_posA = toPosA;
			m_posB = toPosB;
			m_ornA = toOrnA;
			m_ornB = toOrnB;
		}
	}
};

#endif  
