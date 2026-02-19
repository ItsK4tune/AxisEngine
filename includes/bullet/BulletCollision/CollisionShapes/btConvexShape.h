

#ifndef BT_CONVEX_SHAPE_INTERFACE1
#define BT_CONVEX_SHAPE_INTERFACE1

#include "btCollisionShape.h"

#include "LinearMath/btVector3.h"
#include "LinearMath/btTransform.h"
#include "LinearMath/btMatrix3x3.h"
#include "btCollisionMargin.h"
#include "LinearMath/btAlignedAllocator.h"

#define MAX_PREFERRED_PENETRATION_DIRECTIONS 10



ATTRIBUTE_ALIGNED16(class)
btConvexShape : public btCollisionShape
{
public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btConvexShape();

	virtual ~btConvexShape();

	virtual btVector3 localGetSupportingVertex(const btVector3& vec) const = 0;


#ifndef __SPU__
	virtual btVector3 localGetSupportingVertexWithoutMargin(const btVector3& vec) const = 0;
#endif  

	btVector3 localGetSupportVertexWithoutMarginNonVirtual(const btVector3& vec) const;
	btVector3 localGetSupportVertexNonVirtual(const btVector3& vec) const;
	btScalar getMarginNonVirtual() const;
	void getAabbNonVirtual(const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const;

	virtual void project(const btTransform& trans, const btVector3& dir, btScalar& minProj, btScalar& maxProj, btVector3& witnesPtMin, btVector3& witnesPtMax) const;

	
	virtual void batchedUnitVectorGetSupportingVertexWithoutMargin(const btVector3* vectors, btVector3* supportVerticesOut, int numVectors) const = 0;

	
	void getAabb(const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const = 0;

	virtual void getAabbSlow(const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const = 0;

	virtual void setLocalScaling(const btVector3& scaling) = 0;
	virtual const btVector3& getLocalScaling() const = 0;

	virtual void setMargin(btScalar margin) = 0;

	virtual btScalar getMargin() const = 0;

	virtual int getNumPreferredPenetrationDirections() const = 0;

	virtual void getPreferredPenetrationDirection(int index, btVector3& penetrationVector) const = 0;
};

#endif  
