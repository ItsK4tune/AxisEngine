

#ifndef BT_POINT_COLLECTOR_H
#define BT_POINT_COLLECTOR_H

#include "btDiscreteCollisionDetectorInterface.h"

struct btPointCollector : public btDiscreteCollisionDetectorInterface::Result
{
	btVector3 m_normalOnBInWorld;
	btVector3 m_pointInWorld;
	btScalar m_distance;  

	bool m_hasResult;

	btPointCollector()
		: m_distance(btScalar(BT_LARGE_FLOAT)), m_hasResult(false)
	{
	}

	virtual void setShapeIdentifiersA(int partId0, int index0)
	{
		(void)partId0;
		(void)index0;
	}
	virtual void setShapeIdentifiersB(int partId1, int index1)
	{
		(void)partId1;
		(void)index1;
	}

	virtual void addContactPoint(const btVector3& normalOnBInWorld, const btVector3& pointInWorld, btScalar depth)
	{
		if (depth < m_distance)
		{
			m_hasResult = true;
			m_normalOnBInWorld = normalOnBInWorld;
			m_pointInWorld = pointInWorld;
			
			m_distance = depth;
		}
	}
};

#endif  
