

#include "btConvexPointCloudShape.h"
#include "BulletCollision/CollisionShapes/btCollisionMargin.h"

#include "LinearMath/btQuaternion.h"

void btConvexPointCloudShape::setLocalScaling(const btVector3& scaling)
{
	m_localScaling = scaling;
	recalcLocalAabb();
}

#ifndef __SPU__
btVector3 btConvexPointCloudShape::localGetSupportingVertexWithoutMargin(const btVector3& vec0) const
{
	btVector3 supVec(btScalar(0.), btScalar(0.), btScalar(0.));
	btScalar maxDot = btScalar(-BT_LARGE_FLOAT);

	btVector3 vec = vec0;
	btScalar lenSqr = vec.length2();
	if (lenSqr < btScalar(0.0001))
	{
		vec.setValue(1, 0, 0);
	}
	else
	{
		btScalar rlen = btScalar(1.) / btSqrt(lenSqr);
		vec *= rlen;
	}

	if (m_numPoints > 0)
	{
		
		
		int index = (int)vec.maxDot(&m_unscaledPoints[0], m_numPoints, maxDot);  
		return getScaledPoint(index);
	}

	return supVec;
}

void btConvexPointCloudShape::batchedUnitVectorGetSupportingVertexWithoutMargin(const btVector3* vectors, btVector3* supportVerticesOut, int numVectors) const
{
	for (int j = 0; j < numVectors; j++)
	{
		const btVector3& vec = vectors[j] * m_localScaling;  
		btScalar maxDot;
		int index = (int)vec.maxDot(&m_unscaledPoints[0], m_numPoints, maxDot);
		supportVerticesOut[j][3] = btScalar(-BT_LARGE_FLOAT);
		if (0 <= index)
		{
			
			supportVerticesOut[j] = getScaledPoint(index);
			supportVerticesOut[j][3] = maxDot;
		}
	}
}

btVector3 btConvexPointCloudShape::localGetSupportingVertex(const btVector3& vec) const
{
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
}

#endif



int btConvexPointCloudShape::getNumVertices() const
{
	return m_numPoints;
}

int btConvexPointCloudShape::getNumEdges() const
{
	return 0;
}

void btConvexPointCloudShape::getEdge(int i, btVector3& pa, btVector3& pb) const
{
	btAssert(0);
}

void btConvexPointCloudShape::getVertex(int i, btVector3& vtx) const
{
	vtx = m_unscaledPoints[i] * m_localScaling;
}

int btConvexPointCloudShape::getNumPlanes() const
{
	return 0;
}

void btConvexPointCloudShape::getPlane(btVector3&, btVector3&, int) const
{
	btAssert(0);
}


bool btConvexPointCloudShape::isInside(const btVector3&, btScalar) const
{
	btAssert(0);
	return false;
}
