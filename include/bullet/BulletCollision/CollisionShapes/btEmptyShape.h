

#ifndef BT_EMPTY_SHAPE_H
#define BT_EMPTY_SHAPE_H

#include "btConcaveShape.h"

#include "LinearMath/btVector3.h"
#include "LinearMath/btTransform.h"
#include "LinearMath/btMatrix3x3.h"
#include "btCollisionMargin.h"



ATTRIBUTE_ALIGNED16(class)
btEmptyShape : public btConcaveShape
{
public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btEmptyShape();

	virtual ~btEmptyShape();

	
	void getAabb(const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const;

	virtual void setLocalScaling(const btVector3& scaling)
	{
		m_localScaling = scaling;
	}
	virtual const btVector3& getLocalScaling() const
	{
		return m_localScaling;
	}

	virtual void calculateLocalInertia(btScalar mass, btVector3 & inertia) const;

	virtual const char* getName() const
	{
		return "Empty";
	}

	virtual void processAllTriangles(btTriangleCallback*, const btVector3&, const btVector3&) const
	{
	}

protected:
	btVector3 m_localScaling;
};

#endif  
