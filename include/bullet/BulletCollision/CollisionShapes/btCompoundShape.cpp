

#include "btCompoundShape.h"
#include "btCollisionShape.h"
#include "BulletCollision/BroadphaseCollision/btDbvt.h"
#include "LinearMath/btSerializer.h"

btCompoundShape::btCompoundShape(bool enableDynamicAabbTree, const int initialChildCapacity)
	: m_localAabbMin(btScalar(BT_LARGE_FLOAT), btScalar(BT_LARGE_FLOAT), btScalar(BT_LARGE_FLOAT)),
	  m_localAabbMax(btScalar(-BT_LARGE_FLOAT), btScalar(-BT_LARGE_FLOAT), btScalar(-BT_LARGE_FLOAT)),
	  m_dynamicAabbTree(0),
	  m_updateRevision(1),
	  m_collisionMargin(btScalar(0.)),
	  m_localScaling(btScalar(1.), btScalar(1.), btScalar(1.))
{
	m_shapeType = COMPOUND_SHAPE_PROXYTYPE;

	if (enableDynamicAabbTree)
	{
		void* mem = btAlignedAlloc(sizeof(btDbvt), 16);
		m_dynamicAabbTree = new (mem) btDbvt();
		btAssert(mem == m_dynamicAabbTree);
	}

	m_children.reserve(initialChildCapacity);
}

btCompoundShape::~btCompoundShape()
{
	if (m_dynamicAabbTree)
	{
		m_dynamicAabbTree->~btDbvt();
		btAlignedFree(m_dynamicAabbTree);
	}
}

void btCompoundShape::addChildShape(const btTransform& localTransform, btCollisionShape* shape)
{
	m_updateRevision++;
	
	
	btCompoundShapeChild child;
	child.m_node = 0;
	child.m_transform = localTransform;
	child.m_childShape = shape;
	child.m_childShapeType = shape->getShapeType();
	child.m_childMargin = shape->getMargin();

	
	btVector3 localAabbMin, localAabbMax;
	shape->getAabb(localTransform, localAabbMin, localAabbMax);
	for (int i = 0; i < 3; i++)
	{
		if (m_localAabbMin[i] > localAabbMin[i])
		{
			m_localAabbMin[i] = localAabbMin[i];
		}
		if (m_localAabbMax[i] < localAabbMax[i])
		{
			m_localAabbMax[i] = localAabbMax[i];
		}
	}
	if (m_dynamicAabbTree)
	{
		const btDbvtVolume bounds = btDbvtVolume::FromMM(localAabbMin, localAabbMax);
		size_t index = m_children.size();
		child.m_node = m_dynamicAabbTree->insert(bounds, reinterpret_cast<void*>(index));
	}

	m_children.push_back(child);
}

void btCompoundShape::updateChildTransform(int childIndex, const btTransform& newChildTransform, bool shouldRecalculateLocalAabb)
{
	m_children[childIndex].m_transform = newChildTransform;

	if (m_dynamicAabbTree)
	{
		
		btVector3 localAabbMin, localAabbMax;
		m_children[childIndex].m_childShape->getAabb(newChildTransform, localAabbMin, localAabbMax);
		ATTRIBUTE_ALIGNED16(btDbvtVolume)
		bounds = btDbvtVolume::FromMM(localAabbMin, localAabbMax);
		
		m_dynamicAabbTree->update(m_children[childIndex].m_node, bounds);
	}

	if (shouldRecalculateLocalAabb)
	{
		recalculateLocalAabb();
	}
}

void btCompoundShape::removeChildShapeByIndex(int childShapeIndex)
{
	m_updateRevision++;
	btAssert(childShapeIndex >= 0 && childShapeIndex < m_children.size());
	if (m_dynamicAabbTree)
	{
		m_dynamicAabbTree->remove(m_children[childShapeIndex].m_node);
	}
	m_children.swap(childShapeIndex, m_children.size() - 1);
	if (m_dynamicAabbTree)
		m_children[childShapeIndex].m_node->dataAsInt = childShapeIndex;
	m_children.pop_back();
}

void btCompoundShape::removeChildShape(btCollisionShape* shape)
{
	m_updateRevision++;
	
	
	for (int i = m_children.size() - 1; i >= 0; i--)
	{
		if (m_children[i].m_childShape == shape)
		{
			removeChildShapeByIndex(i);
		}
	}

	recalculateLocalAabb();
}

void btCompoundShape::recalculateLocalAabb()
{
	
	

	m_localAabbMin = btVector3(btScalar(BT_LARGE_FLOAT), btScalar(BT_LARGE_FLOAT), btScalar(BT_LARGE_FLOAT));
	m_localAabbMax = btVector3(btScalar(-BT_LARGE_FLOAT), btScalar(-BT_LARGE_FLOAT), btScalar(-BT_LARGE_FLOAT));

	
	for (int j = 0; j < m_children.size(); j++)
	{
		btVector3 localAabbMin, localAabbMax;
		m_children[j].m_childShape->getAabb(m_children[j].m_transform, localAabbMin, localAabbMax);
		for (int i = 0; i < 3; i++)
		{
			if (m_localAabbMin[i] > localAabbMin[i])
				m_localAabbMin[i] = localAabbMin[i];
			if (m_localAabbMax[i] < localAabbMax[i])
				m_localAabbMax[i] = localAabbMax[i];
		}
	}
}


void btCompoundShape::getAabb(const btTransform& trans, btVector3& aabbMin, btVector3& aabbMax) const
{
	btVector3 localHalfExtents = btScalar(0.5) * (m_localAabbMax - m_localAabbMin);
	btVector3 localCenter = btScalar(0.5) * (m_localAabbMax + m_localAabbMin);

	
	if (!m_children.size())
	{
		localHalfExtents.setValue(0, 0, 0);
		localCenter.setValue(0, 0, 0);
	}
	localHalfExtents += btVector3(getMargin(), getMargin(), getMargin());

	btMatrix3x3 abs_b = trans.getBasis().absolute();

	btVector3 center = trans(localCenter);

	btVector3 extent = localHalfExtents.dot3(abs_b[0], abs_b[1], abs_b[2]);
	aabbMin = center - extent;
	aabbMax = center + extent;
}

void btCompoundShape::calculateLocalInertia(btScalar mass, btVector3& inertia) const
{
	
	btTransform ident;
	ident.setIdentity();
	btVector3 aabbMin, aabbMax;
	getAabb(ident, aabbMin, aabbMax);

	btVector3 halfExtents = (aabbMax - aabbMin) * btScalar(0.5);

	btScalar lx = btScalar(2.) * (halfExtents.x());
	btScalar ly = btScalar(2.) * (halfExtents.y());
	btScalar lz = btScalar(2.) * (halfExtents.z());

	inertia[0] = mass / (btScalar(12.0)) * (ly * ly + lz * lz);
	inertia[1] = mass / (btScalar(12.0)) * (lx * lx + lz * lz);
	inertia[2] = mass / (btScalar(12.0)) * (lx * lx + ly * ly);
}

void btCompoundShape::calculatePrincipalAxisTransform(const btScalar* masses, btTransform& principal, btVector3& inertia) const
{
	int n = m_children.size();

	btScalar totalMass = 0;
	btVector3 center(0, 0, 0);
	int k;

	for (k = 0; k < n; k++)
	{
		btAssert(masses[k] > 0);
		center += m_children[k].m_transform.getOrigin() * masses[k];
		totalMass += masses[k];
	}

	btAssert(totalMass > 0);

	center /= totalMass;
	principal.setOrigin(center);

	btMatrix3x3 tensor(0, 0, 0, 0, 0, 0, 0, 0, 0);
	for (k = 0; k < n; k++)
	{
		btVector3 i;
		m_children[k].m_childShape->calculateLocalInertia(masses[k], i);

		const btTransform& t = m_children[k].m_transform;
		btVector3 o = t.getOrigin() - center;

		
		btMatrix3x3 j = t.getBasis().transpose();
		j[0] *= i[0];
		j[1] *= i[1];
		j[2] *= i[2];
		j = t.getBasis() * j;

		
		tensor[0] += j[0];
		tensor[1] += j[1];
		tensor[2] += j[2];

		
		btScalar o2 = o.length2();
		j[0].setValue(o2, 0, 0);
		j[1].setValue(0, o2, 0);
		j[2].setValue(0, 0, o2);
		j[0] += o * -o.x();
		j[1] += o * -o.y();
		j[2] += o * -o.z();

		
		tensor[0] += masses[k] * j[0];
		tensor[1] += masses[k] * j[1];
		tensor[2] += masses[k] * j[2];
	}

	tensor.diagonalize(principal.getBasis(), btScalar(0.00001), 20);
	inertia.setValue(tensor[0][0], tensor[1][1], tensor[2][2]);
}

void btCompoundShape::setLocalScaling(const btVector3& scaling)
{
	for (int i = 0; i < m_children.size(); i++)
	{
		btTransform childTrans = getChildTransform(i);
		btVector3 childScale = m_children[i].m_childShape->getLocalScaling();
		
		childScale = childScale * scaling / m_localScaling;
		m_children[i].m_childShape->setLocalScaling(childScale);
		childTrans.setOrigin((childTrans.getOrigin()) * scaling / m_localScaling);
		updateChildTransform(i, childTrans, false);
	}

	m_localScaling = scaling;
	recalculateLocalAabb();
}

void btCompoundShape::createAabbTreeFromChildren()
{
	if (!m_dynamicAabbTree)
	{
		void* mem = btAlignedAlloc(sizeof(btDbvt), 16);
		m_dynamicAabbTree = new (mem) btDbvt();
		btAssert(mem == m_dynamicAabbTree);

		for (int index = 0; index < m_children.size(); index++)
		{
			btCompoundShapeChild& child = m_children[index];

			
			btVector3 localAabbMin, localAabbMax;
			child.m_childShape->getAabb(child.m_transform, localAabbMin, localAabbMax);

			const btDbvtVolume bounds = btDbvtVolume::FromMM(localAabbMin, localAabbMax);
			size_t index2 = index;
			child.m_node = m_dynamicAabbTree->insert(bounds, reinterpret_cast<void*>(index2));
		}
	}
}


const char* btCompoundShape::serialize(void* dataBuffer, btSerializer* serializer) const
{
	btCompoundShapeData* shapeData = (btCompoundShapeData*)dataBuffer;
	btCollisionShape::serialize(&shapeData->m_collisionShapeData, serializer);

	shapeData->m_collisionMargin = float(m_collisionMargin);
	shapeData->m_numChildShapes = m_children.size();
	shapeData->m_childShapePtr = 0;
	if (shapeData->m_numChildShapes)
	{
		btChunk* chunk = serializer->allocate(sizeof(btCompoundShapeChildData), shapeData->m_numChildShapes);
		btCompoundShapeChildData* memPtr = (btCompoundShapeChildData*)chunk->m_oldPtr;
		shapeData->m_childShapePtr = (btCompoundShapeChildData*)serializer->getUniquePointer(memPtr);

		for (int i = 0; i < shapeData->m_numChildShapes; i++, memPtr++)
		{
			memPtr->m_childMargin = float(m_children[i].m_childMargin);
			memPtr->m_childShape = (btCollisionShapeData*)serializer->getUniquePointer(m_children[i].m_childShape);
			
			if (!serializer->findPointer(m_children[i].m_childShape))
			{
				btChunk* chunk = serializer->allocate(m_children[i].m_childShape->calculateSerializeBufferSize(), 1);
				const char* structType = m_children[i].m_childShape->serialize(chunk->m_oldPtr, serializer);
				serializer->finalizeChunk(chunk, structType, BT_SHAPE_CODE, m_children[i].m_childShape);
			}

			memPtr->m_childShapeType = m_children[i].m_childShapeType;
			m_children[i].m_transform.serializeFloat(memPtr->m_transform);
		}
		serializer->finalizeChunk(chunk, "btCompoundShapeChildData", BT_ARRAY_CODE, chunk->m_oldPtr);
	}
	return "btCompoundShapeData";
}
