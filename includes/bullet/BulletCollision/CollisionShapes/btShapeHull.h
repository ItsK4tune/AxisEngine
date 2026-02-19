



#ifndef BT_SHAPE_HULL_H
#define BT_SHAPE_HULL_H

#include "LinearMath/btAlignedObjectArray.h"
#include "BulletCollision/CollisionShapes/btConvexShape.h"




ATTRIBUTE_ALIGNED16(class)
btShapeHull
{
protected:
	btAlignedObjectArray<btVector3> m_vertices;
	btAlignedObjectArray<unsigned int> m_indices;
	unsigned int m_numIndices;
	const btConvexShape* m_shape;

	static btVector3* getUnitSpherePoints(int highres = 0);

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btShapeHull(const btConvexShape* shape);
	~btShapeHull();

	bool buildHull(btScalar margin, int highres = 0);

	int numTriangles() const;
	int numVertices() const;
	int numIndices() const;

	const btVector3* getVertexPointer() const
	{
		return &m_vertices[0];
	}
	const unsigned int* getIndexPointer() const
	{
		return &m_indices[0];
	}
};

#endif  
