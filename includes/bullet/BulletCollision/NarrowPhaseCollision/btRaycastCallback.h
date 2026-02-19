

#ifndef BT_RAYCAST_TRI_CALLBACK_H
#define BT_RAYCAST_TRI_CALLBACK_H

#include "BulletCollision/CollisionShapes/btTriangleCallback.h"
#include "LinearMath/btTransform.h"
struct btBroadphaseProxy;
class btConvexShape;

class btTriangleRaycastCallback : public btTriangleCallback
{
public:
	
	btVector3 m_from;
	btVector3 m_to;

	
	enum EFlags
	{
		kF_None = 0,
		kF_FilterBackfaces = 1 << 0,
		kF_KeepUnflippedNormal = 1 << 1,             
													 
		kF_UseSubSimplexConvexCastRaytest = 1 << 2,  
		kF_UseGjkConvexCastRaytest = 1 << 3,
		kF_DisableHeightfieldAccelerator  = 1 << 4, 
		kF_Terminator = 0xFFFFFFFF
	};
	unsigned int m_flags;

	btScalar m_hitFraction;

	btTriangleRaycastCallback(const btVector3& from, const btVector3& to, unsigned int flags = 0);

	virtual void processTriangle(btVector3* triangle, int partId, int triangleIndex);

	virtual btScalar reportHit(const btVector3& hitNormalLocal, btScalar hitFraction, int partId, int triangleIndex) = 0;
};

class btTriangleConvexcastCallback : public btTriangleCallback
{
public:
	const btConvexShape* m_convexShape;
	btTransform m_convexShapeFrom;
	btTransform m_convexShapeTo;
	btTransform m_triangleToWorld;
	btScalar m_hitFraction;
	btScalar m_triangleCollisionMargin;
	btScalar m_allowedPenetration;

	btTriangleConvexcastCallback(const btConvexShape* convexShape, const btTransform& convexShapeFrom, const btTransform& convexShapeTo, const btTransform& triangleToWorld, const btScalar triangleCollisionMargin);

	virtual void processTriangle(btVector3* triangle, int partId, int triangleIndex);

	virtual btScalar reportHit(const btVector3& hitNormalLocal, const btVector3& hitPointLocal, btScalar hitFraction, int partId, int triangleIndex) = 0;
};

#endif  
