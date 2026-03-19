

#ifndef BT_MINKOWSKI_SUM_SHAPE_H
#define BT_MINKOWSKI_SUM_SHAPE_H

#include "btConvexInternalShape.h"
#include "BulletCollision/BroadphaseCollision/btBroadphaseProxy.h"  


ATTRIBUTE_ALIGNED16(class)
btMinkowskiSumShape : public btConvexInternalShape
{
	btTransform m_transA;
	btTransform m_transB;
	const btConvexShape* m_shapeA;
	const btConvexShape* m_shapeB;

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btMinkowskiSumShape(const btConvexShape* shapeA, const btConvexShape* shapeB);

	virtual btVector3 localGetSupportingVertexWithoutMargin(const btVector3& vec) const;

	virtual void batchedUnitVectorGetSupportingVertexWithoutMargin(const btVector3* vectors, btVector3* supportVerticesOut, int numVectors) const;

	virtual void calculateLocalInertia(btScalar mass, btVector3 & inertia) const;

	void setTransformA(const btTransform& transA) { m_transA = transA; }
	void setTransformB(const btTransform& transB) { m_transB = transB; }

	const btTransform& getTransformA() const { return m_transA; }
	const btTransform& getTransformB() const { return m_transB; }

	
	const btTransform& GetTransformB() const { return m_transB; }

	virtual btScalar getMargin() const;

	const btConvexShape* getShapeA() const { return m_shapeA; }
	const btConvexShape* getShapeB() const { return m_shapeB; }

	virtual const char* getName() const
	{
		return "MinkowskiSum";
	}
};

#endif  
