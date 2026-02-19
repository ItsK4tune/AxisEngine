

#ifndef GJK_COLLISION_DESCRIPTION_H
#define GJK_COLLISION_DESCRIPTION_H

#include "LinearMath/btVector3.h"

struct btGjkCollisionDescription
{
	btVector3 m_firstDir;
	int m_maxGjkIterations;
	btScalar m_maximumDistanceSquared;
	btScalar m_gjkRelError2;
	btGjkCollisionDescription()
		: m_firstDir(0, 1, 0),
		  m_maxGjkIterations(1000),
		  m_maximumDistanceSquared(1e30f),
		  m_gjkRelError2(1.0e-6)
	{
	}
	virtual ~btGjkCollisionDescription()
	{
	}
};

#endif  
