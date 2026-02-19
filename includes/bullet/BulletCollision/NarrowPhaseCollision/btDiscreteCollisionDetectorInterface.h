

#ifndef BT_DISCRETE_COLLISION_DETECTOR1_INTERFACE_H
#define BT_DISCRETE_COLLISION_DETECTOR1_INTERFACE_H

#include "LinearMath/btTransform.h"
#include "LinearMath/btVector3.h"






struct btDiscreteCollisionDetectorInterface
{
	struct Result
	{
		virtual ~Result() {}

		
		virtual void setShapeIdentifiersA(int partId0, int index0) = 0;
		virtual void setShapeIdentifiersB(int partId1, int index1) = 0;
		virtual void addContactPoint(const btVector3& normalOnBInWorld, const btVector3& pointInWorld, btScalar depth) = 0;
	};

	struct ClosestPointInput
	{
		ClosestPointInput()
			: m_maximumDistanceSquared(btScalar(BT_LARGE_FLOAT))
		{
		}

		btTransform m_transformA;
		btTransform m_transformB;
		btScalar m_maximumDistanceSquared;
	};

	virtual ~btDiscreteCollisionDetectorInterface(){};

	
	
	
	
	virtual void getClosestPoints(const ClosestPointInput& input, Result& output, class btIDebugDraw* debugDraw, bool swapResults = false) = 0;
};

struct btStorageResult : public btDiscreteCollisionDetectorInterface::Result
{
	btVector3 m_normalOnSurfaceB;
	btVector3 m_closestPointInB;
	btScalar m_distance;  

protected:
	btStorageResult() : m_distance(btScalar(BT_LARGE_FLOAT))
	{
	}

public:
	virtual ~btStorageResult(){};

	virtual void addContactPoint(const btVector3& normalOnBInWorld, const btVector3& pointInWorld, btScalar depth)
	{
		if (depth < m_distance)
		{
			m_normalOnSurfaceB = normalOnBInWorld;
			m_closestPointInB = pointInWorld;
			m_distance = depth;
		}
	}
};

#endif  
