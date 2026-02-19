
#ifndef BT_BOX_BOX_DETECTOR_H
#define BT_BOX_BOX_DETECTOR_H

class btBoxShape;
#include "BulletCollision/NarrowPhaseCollision/btDiscreteCollisionDetectorInterface.h"



struct btBoxBoxDetector : public btDiscreteCollisionDetectorInterface
{
	const btBoxShape* m_box1;
	const btBoxShape* m_box2;

public:
	btBoxBoxDetector(const btBoxShape* box1, const btBoxShape* box2);

	virtual ~btBoxBoxDetector(){};

	virtual void getClosestPoints(const ClosestPointInput& input, Result& output, class btIDebugDraw* debugDraw, bool swapResults = false);
};

#endif  
