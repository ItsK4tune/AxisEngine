

#ifndef BT_CONVEX_HULL_SHAPE_H
#define BT_CONVEX_HULL_SHAPE_H

#include "btPolyhedralConvexShape.h"
#include "BulletCollision/BroadphaseCollision/btBroadphaseProxy.h"  
#include "LinearMath/btAlignedObjectArray.h"



ATTRIBUTE_ALIGNED16(class)
btConvexHullShape : public btPolyhedralConvexAabbCachingShape
{
protected:
	btAlignedObjectArray<btVector3> m_unscaledPoints;

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	
	
	
	btConvexHullShape(const btScalar* points = 0, int numPoints = 0, int stride = sizeof(btVector3));

	void addPoint(const btVector3& point, bool recalculateLocalAabb = true);

	btVector3* getUnscaledPoints()
	{
		return &m_unscaledPoints[0];
	}

	const btVector3* getUnscaledPoints() const
	{
		return &m_unscaledPoints[0];
	}

	
	const btVector3* getPoints() const
	{
		return getUnscaledPoints();
	}

	void optimizeConvexHull();

	SIMD_FORCE_INLINE btVector3 getScaledPoint(int i) const
	{
		return m_unscaledPoints[i] * m_localScaling;
	}

	SIMD_FORCE_INLINE int getNumPoints() const
	{
		return m_unscaledPoints.size();
	}

	virtual btVector3 localGetSupportingVertex(const btVector3& vec) const;
	virtual btVector3 localGetSupportingVertexWithoutMargin(const btVector3& vec) const;
	virtual void batchedUnitVectorGetSupportingVertexWithoutMargin(const btVector3* vectors, btVector3* supportVerticesOut, int numVectors) const;

	virtual void project(const btTransform& trans, const btVector3& dir, btScalar& minProj, btScalar& maxProj, btVector3& witnesPtMin, btVector3& witnesPtMax) const;

	
	virtual const char* getName() const { return "Convex"; }

	virtual int getNumVertices() const;
	virtual int getNumEdges() const;
	virtual void getEdge(int i, btVector3& pa, btVector3& pb) const;
	virtual void getVertex(int i, btVector3& vtx) const;
	virtual int getNumPlanes() const;
	virtual void getPlane(btVector3 & planeNormal, btVector3 & planeSupport, int i) const;
	virtual bool isInside(const btVector3& pt, btScalar tolerance) const;

	
	virtual void setLocalScaling(const btVector3& scaling);

	virtual int calculateSerializeBufferSize() const;

	
	virtual const char* serialize(void* dataBuffer, btSerializer* serializer) const;
};




struct	btConvexHullShapeData
{
	btConvexInternalShapeData	m_convexInternalShapeData;

	btVector3FloatData	*m_unscaledPointsFloatPtr;
	btVector3DoubleData	*m_unscaledPointsDoublePtr;

	int		m_numUnscaledPoints;
	char m_padding3[4];

};



SIMD_FORCE_INLINE int btConvexHullShape::calculateSerializeBufferSize() const
{
	return sizeof(btConvexHullShapeData);
}

#endif  
