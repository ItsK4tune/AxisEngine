

#ifndef BT_SPHERE_TRIANGLE_DETECTOR_H
#define BT_SPHERE_TRIANGLE_DETECTOR_H

#include "BulletCollision/NarrowPhaseCollision/btDiscreteCollisionDetectorInterface.h"

class btSphereShape;
class btTriangleShape;


struct SphereTriangleDetector : public btDiscreteCollisionDetectorInterface
{
	virtual void getClosestPoints(const ClosestPointInput& input, Result& output, class btIDebugDraw* debugDraw, bool swapResults = false);

	SphereTriangleDetector(btSphereShape* sphere, btTriangleShape* triangle, btScalar contactBreakingThreshold);

	virtual ~SphereTriangleDetector(){};

	bool collide(const btVector3& sphereCenter, btVector3& point, btVector3& resultNormal, btScalar& depth, btScalar& timeOfImpact, btScalar contactBreakingThreshold);

private:
	bool pointInTriangle(const btVector3 vertices[], const btVector3& normal, btVector3* p);
	bool facecontains(const btVector3& p, const btVector3* vertices, btVector3& normal);

	btSphereShape* m_sphere;
	btTriangleShape* m_triangle;
	btScalar m_contactBreakingThreshold;
};
#endif  
