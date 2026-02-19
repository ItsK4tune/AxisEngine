

#ifndef BT_COMPOUND_SHAPE_H
#define BT_COMPOUND_SHAPE_H

#include "btCollisionShape.h"

#include "LinearMath/btVector3.h"
#include "LinearMath/btTransform.h"
#include "LinearMath/btMatrix3x3.h"
#include "btCollisionMargin.h"
#include "LinearMath/btAlignedObjectArray.h"


struct btDbvt;

ATTRIBUTE_ALIGNED16(struct)
btCompoundShapeChild
{
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btTransform m_transform;
	btCollisionShape* m_childShape;
	int m_childShapeType;
	btScalar m_childMargin;
	struct btDbvtNode* m_node;
};

SIMD_FORCE_INLINE bool operator==(const btCompoundShapeChild& c1, const btCompoundShapeChild& c2)
{
	return (c1.m_transform == c2.m_transform &&
			c1.m_childShape == c2.m_childShape &&
			c1.m_childShapeType == c2.m_childShapeType &&
			c1.m_childMargin == c2.m_childMargin);
}






ATTRIBUTE_ALIGNED16(class)
btCompoundShape : public btCollisionShape
{
protected:
	btAlignedObjectArray<btCompoundShapeChild> m_children;
	btVector3 m_localAabbMin;
	btVector3 m_localAabbMax;

	btDbvt* m_dynamicAabbTree;

	
	int m_updateRevision;

	btScalar m_collisionMargin;

	btVector3 m_localScaling;

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	explicit btCompoundShape(bool enableDynamicAabbTree = true, const int initialChildCapacity = 0);

	virtual ~btCompoundShape();

	void addChildShape(const btTransform& localTransform, btCollisionShape* shape);

	
	virtual void removeChildShape(btCollisionShape * shape);

	void removeChildShapeByIndex(int childShapeindex);

	int getNumChildShapes() const
	{
		return int(m_children.size());
	}

	btCollisionShape* getChildShape(int index)
	{
		return m_children[index].m_childShape;
	}
	const btCollisionShape* getChildShape(int index) const
	{
		return m_children[index].m_childShape;
	}

	btTransform& getChildTransform(int index)
	{
		return m_children[index].m_transform;
	}
	const btTransform& getChildTransform(int index) const
	{
		return m_children[index].m_transform;
	}

	
	void updateChildTransform(int childIndex, const btTransform& newChildTransform, bool shouldRecalculateLocalAabb = true);

	btCompoundShapeChild* getChildList()
	{
		return &m_children[0];
	}

	
	virtual void getAabb(const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const;

	
	virtual void recalculateLocalAabb();

	virtual void setLocalScaling(const btVector3& scaling);

	virtual const btVector3& getLocalScaling() const
	{
		return m_localScaling;
	}

	virtual void calculateLocalInertia(btScalar mass, btVector3 & inertia) const;

	virtual void setMargin(btScalar margin)
	{
		m_collisionMargin = margin;
	}
	virtual btScalar getMargin() const
	{
		return m_collisionMargin;
	}
	virtual const char* getName() const
	{
		return "Compound";
	}

	const btDbvt* getDynamicAabbTree() const
	{
		return m_dynamicAabbTree;
	}

	btDbvt* getDynamicAabbTree()
	{
		return m_dynamicAabbTree;
	}

	void createAabbTreeFromChildren();

	
	
	
	
	
	void calculatePrincipalAxisTransform(const btScalar* masses, btTransform& principal, btVector3& inertia) const;

	int getUpdateRevision() const
	{
		return m_updateRevision;
	}

	virtual int calculateSerializeBufferSize() const;

	
	virtual const char* serialize(void* dataBuffer, btSerializer* serializer) const;
};




struct btCompoundShapeChildData
{
	btTransformFloatData	m_transform;
	btCollisionShapeData	*m_childShape;
	int						m_childShapeType;
	float					m_childMargin;
};


struct	btCompoundShapeData
{
	btCollisionShapeData		m_collisionShapeData;

	btCompoundShapeChildData	*m_childShapePtr;

	int							m_numChildShapes;

	float	m_collisionMargin;

};



SIMD_FORCE_INLINE int btCompoundShape::calculateSerializeBufferSize() const
{
	return sizeof(btCompoundShapeData);
}

#endif  
