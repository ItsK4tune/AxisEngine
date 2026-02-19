

#include "btConvexInternalShape.h"

btConvexInternalShape::btConvexInternalShape()
	: m_localScaling(btScalar(1.), btScalar(1.), btScalar(1.)),
	  m_collisionMargin(CONVEX_DISTANCE_MARGIN)
{
}

void btConvexInternalShape::setLocalScaling(const btVector3& scaling)
{
	m_localScaling = scaling.absolute();
}

void btConvexInternalShape::getAabbSlow(const btTransform& trans, btVector3& minAabb, btVector3& maxAabb) const
{
#ifndef __SPU__
	
	btScalar margin = getMargin();
	for (int i = 0; i < 3; i++)
	{
		btVector3 vec(btScalar(0.), btScalar(0.), btScalar(0.));
		vec[i] = btScalar(1.);

		btVector3 sv = localGetSupportingVertex(vec * trans.getBasis());

		btVector3 tmp = trans(sv);
		maxAabb[i] = tmp[i] + margin;
		vec[i] = btScalar(-1.);
		tmp = trans(localGetSupportingVertex(vec * trans.getBasis()));
		minAabb[i] = tmp[i] - margin;
	}
#endif
}

btVector3 btConvexInternalShape::localGetSupportingVertex(const btVector3& vec) const
{
#ifndef __SPU__

	btVector3 supVertex = localGetSupportingVertexWithoutMargin(vec);

	if (getMargin() != btScalar(0.))
	{
		btVector3 vecnorm = vec;
		if (vecnorm.length2() < (SIMD_EPSILON * SIMD_EPSILON))
		{
			vecnorm.setValue(btScalar(-1.), btScalar(-1.), btScalar(-1.));
		}
		vecnorm.normalize();
		supVertex += getMargin() * vecnorm;
	}
	return supVertex;

#else
	btAssert(0);
	return btVector3(0, 0, 0);
#endif  
}

btConvexInternalAabbCachingShape::btConvexInternalAabbCachingShape()
	: btConvexInternalShape(),
	  m_localAabbMin(1, 1, 1),
	  m_localAabbMax(-1, -1, -1),
	  m_isLocalAabbValid(false)
{
}

void btConvexInternalAabbCachingShape::getAabb(const btTransform& trans, btVector3& aabbMin, btVector3& aabbMax) const
{
	getNonvirtualAabb(trans, aabbMin, aabbMax, getMargin());
}

void btConvexInternalAabbCachingShape::setLocalScaling(const btVector3& scaling)
{
	btConvexInternalShape::setLocalScaling(scaling);
	recalcLocalAabb();
}

void btConvexInternalAabbCachingShape::recalcLocalAabb()
{
	m_isLocalAabbValid = true;

#if 1
	static const btVector3 _directions[] =
		{
			btVector3(1., 0., 0.),
			btVector3(0., 1., 0.),
			btVector3(0., 0., 1.),
			btVector3(-1., 0., 0.),
			btVector3(0., -1., 0.),
			btVector3(0., 0., -1.)};

	btVector3 _supporting[] =
		{
			btVector3(0., 0., 0.),
			btVector3(0., 0., 0.),
			btVector3(0., 0., 0.),
			btVector3(0., 0., 0.),
			btVector3(0., 0., 0.),
			btVector3(0., 0., 0.)};

	batchedUnitVectorGetSupportingVertexWithoutMargin(_directions, _supporting, 6);

	for (int i = 0; i < 3; ++i)
	{
		m_localAabbMax[i] = _supporting[i][i] + m_collisionMargin;
		m_localAabbMin[i] = _supporting[i + 3][i] - m_collisionMargin;
	}

#else

	for (int i = 0; i < 3; i++)
	{
		btVector3 vec(btScalar(0.), btScalar(0.), btScalar(0.));
		vec[i] = btScalar(1.);
		btVector3 tmp = localGetSupportingVertex(vec);
		m_localAabbMax[i] = tmp[i] + m_collisionMargin;
		vec[i] = btScalar(-1.);
		tmp = localGetSupportingVertex(vec);
		m_localAabbMin[i] = tmp[i] - m_collisionMargin;
	}
#endif
}
