



#ifndef _BT_POLYHEDRAL_FEATURES_H
#define _BT_POLYHEDRAL_FEATURES_H

#include "LinearMath/btTransform.h"
#include "LinearMath/btAlignedObjectArray.h"

#define TEST_INTERNAL_OBJECTS 1

struct btFace
{
	btAlignedObjectArray<int> m_indices;
	
	btScalar m_plane[4];
};

ATTRIBUTE_ALIGNED16(class)
btConvexPolyhedron
{
public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btConvexPolyhedron();
	virtual ~btConvexPolyhedron();

	btAlignedObjectArray<btVector3> m_vertices;
	btAlignedObjectArray<btFace> m_faces;
	btAlignedObjectArray<btVector3> m_uniqueEdges;

	btVector3 m_localCenter;
	btVector3 m_extents;
	btScalar m_radius;
	btVector3 mC;
	btVector3 mE;

	void initialize();
	void initialize2();
	bool testContainment() const;

	void project(const btTransform& trans, const btVector3& dir, btScalar& minProj, btScalar& maxProj, btVector3& witnesPtMin, btVector3& witnesPtMax) const;
};

#endif  
