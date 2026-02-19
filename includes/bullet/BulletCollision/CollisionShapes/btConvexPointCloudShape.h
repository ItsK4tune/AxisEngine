

#ifndef BT_CONVEX_POINT_CLOUD_SHAPE_H
#define BT_CONVEX_POINT_CLOUD_SHAPE_H

#include "btPolyhedralConvexShape.h"
#include "BulletCollision/BroadphaseCollision/btBroadphaseProxy.h"  
#include "LinearMath/btAlignedObjectArray.h"


ATTRIBUTE_ALIGNED16(class)
btConvexPointCloudShape : public btPolyhedralConvexAabbCachingShape
{
	btVector3* m_unscaledPoints;
	int m_numPoints;

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btConvexPointCloudShape()
	{
		m_localScaling.setValue(1.f, 1.f, 1.f);
		m_shapeType = CONVEX_POINT_CLOUD_SHAPE_PROXYTYPE;
		m_unscaledPoints = 0;
		m_numPoints = 0;
	}

	btConvexPointCloudShape(btVector3 * points, int numPoints, const btVector3& localScaling, bool computeAabb = true)
	{
		m_localScaling = localScaling;
		m_shapeType = CONVEX_POINT_CLOUD_SHAPE_PROXYTYPE;
		m_unscaledPoints = points;
		m_numPoints = numPoints;

		if (computeAabb)
			recalcLocalAabb();
	}

	void setPoints(btVector3 * points, int numPoints, bool computeAabb = true, const btVector3& localScaling = btVector3(1.f, 1.f, 1.f))
	{
		m_unscaledPoints = points;
		m_numPoints = numPoints;
		m_localScaling = localScaling;

		if (computeAabb)
			recalcLocalAabb();
	}

	SIMD_FORCE_INLINE btVector3* getUnscaledPoints()
	{
		return m_unscaledPoints;
	}

	SIMD_FORCE_INLINE const btVector3* getUnscaledPoints() const
	{
		return m_unscaledPoints;
	}

	SIMD_FORCE_INLINE int getNumPoints() const
	{
		return m_numPoints;
	}

	SIMD_FORCE_INLINE btVector3 getScaledPoint(int index) const
	{
		return m_unscaledPoints[index] * m_localScaling;
	}

#ifndef __SPU__
	virtual btVector3 localGetSupportingVertex(const btVector3& vec) const;
	virtual btVector3 localGetSupportingVertexWithoutMargin(const btVector3& vec) const;
	virtual void batchedUnitVectorGetSupportingVertexWithoutMargin(const btVector3* vectors, btVector3* supportVerticesOut, int numVectors) const;
#endif

	
	virtual const char* getName() const { return "ConvexPointCloud"; }

	virtual int getNumVertices() const;
	virtual int getNumEdges() const;
	virtual void getEdge(int i, btVector3& pa, btVector3& pb) const;
	virtual void getVertex(int i, btVector3& vtx) const;
	virtual int getNumPlanes() const;
	virtual void getPlane(btVector3 & planeNormal, btVector3 & planeSupport, int i) const;
	virtual bool isInside(const btVector3& pt, btScalar tolerance) const;

	
	virtual void setLocalScaling(const btVector3& scaling);
};

#endif  
