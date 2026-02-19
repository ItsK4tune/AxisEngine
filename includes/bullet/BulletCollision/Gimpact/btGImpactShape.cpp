

#include "btGImpactShape.h"
#include "btGImpactMassUtil.h"

btGImpactMeshShapePart::btGImpactMeshShapePart(btStridingMeshInterface* meshInterface, int part)
{
	
	
	
	m_primitive_manager.m_meshInterface = meshInterface;
	m_primitive_manager.m_part = part;
	m_box_set.setPrimitiveManager(&m_primitive_manager);
#if BT_THREADSAFE
	
	
	
	
	
	
	
	
	
	m_primitive_manager.lock();
#endif
}

btGImpactMeshShapePart::~btGImpactMeshShapePart()
{
	
#if BT_THREADSAFE
	m_primitive_manager.unlock();
#endif
}

void btGImpactMeshShapePart::lockChildShapes() const
{
	
#if !BT_THREADSAFE
	
	void* dummy = (void*)(m_box_set.getPrimitiveManager());
	TrimeshPrimitiveManager* dummymanager = static_cast<TrimeshPrimitiveManager*>(dummy);
	dummymanager->lock();
#endif
}

void btGImpactMeshShapePart::unlockChildShapes() const
{
	
#if !BT_THREADSAFE
	
	void* dummy = (void*)(m_box_set.getPrimitiveManager());
	TrimeshPrimitiveManager* dummymanager = static_cast<TrimeshPrimitiveManager*>(dummy);
	dummymanager->unlock();
#endif
}

#define CALC_EXACT_INERTIA 1

void btGImpactCompoundShape::calculateLocalInertia(btScalar mass, btVector3& inertia) const
{
	lockChildShapes();
#ifdef CALC_EXACT_INERTIA
	inertia.setValue(0.f, 0.f, 0.f);

	int i = this->getNumChildShapes();
	btScalar shapemass = mass / btScalar(i);

	while (i--)
	{
		btVector3 temp_inertia;
		m_childShapes[i]->calculateLocalInertia(shapemass, temp_inertia);
		if (childrenHasTransform())
		{
			inertia = gim_inertia_add_transformed(inertia, temp_inertia, m_childTransforms[i]);
		}
		else
		{
			inertia = gim_inertia_add_transformed(inertia, temp_inertia, btTransform::getIdentity());
		}
	}

#else

	

	btScalar lx = m_localAABB.m_max[0] - m_localAABB.m_min[0];
	btScalar ly = m_localAABB.m_max[1] - m_localAABB.m_min[1];
	btScalar lz = m_localAABB.m_max[2] - m_localAABB.m_min[2];
	const btScalar x2 = lx * lx;
	const btScalar y2 = ly * ly;
	const btScalar z2 = lz * lz;
	const btScalar scaledmass = mass * btScalar(0.08333333);

	inertia = scaledmass * (btVector3(y2 + z2, x2 + z2, x2 + y2));

#endif
	unlockChildShapes();
}

void btGImpactMeshShapePart::calculateLocalInertia(btScalar mass, btVector3& inertia) const
{
	lockChildShapes();

#ifdef CALC_EXACT_INERTIA
	inertia.setValue(0.f, 0.f, 0.f);

	int i = this->getVertexCount();
	btScalar pointmass = mass / btScalar(i);

	while (i--)
	{
		btVector3 pointintertia;
		this->getVertex(i, pointintertia);
		pointintertia = gim_get_point_inertia(pointintertia, pointmass);
		inertia += pointintertia;
	}

#else

	

	btScalar lx = m_localAABB.m_max[0] - m_localAABB.m_min[0];
	btScalar ly = m_localAABB.m_max[1] - m_localAABB.m_min[1];
	btScalar lz = m_localAABB.m_max[2] - m_localAABB.m_min[2];
	const btScalar x2 = lx * lx;
	const btScalar y2 = ly * ly;
	const btScalar z2 = lz * lz;
	const btScalar scaledmass = mass * btScalar(0.08333333);

	inertia = scaledmass * (btVector3(y2 + z2, x2 + z2, x2 + y2));

#endif

	unlockChildShapes();
}

void btGImpactMeshShape::calculateLocalInertia(btScalar mass, btVector3& inertia) const
{
#ifdef CALC_EXACT_INERTIA
	inertia.setValue(0.f, 0.f, 0.f);

	int i = this->getMeshPartCount();
	btScalar partmass = mass / btScalar(i);

	while (i--)
	{
		btVector3 partinertia;
		getMeshPart(i)->calculateLocalInertia(partmass, partinertia);
		inertia += partinertia;
	}

#else

	

	btScalar lx = m_localAABB.m_max[0] - m_localAABB.m_min[0];
	btScalar ly = m_localAABB.m_max[1] - m_localAABB.m_min[1];
	btScalar lz = m_localAABB.m_max[2] - m_localAABB.m_min[2];
	const btScalar x2 = lx * lx;
	const btScalar y2 = ly * ly;
	const btScalar z2 = lz * lz;
	const btScalar scaledmass = mass * btScalar(0.08333333);

	inertia = scaledmass * (btVector3(y2 + z2, x2 + z2, x2 + y2));

#endif
}

void btGImpactMeshShape::rayTest(const btVector3& rayFrom, const btVector3& rayTo, btCollisionWorld::RayResultCallback& resultCallback) const
{
}

void btGImpactMeshShapePart::processAllTrianglesRay(btTriangleCallback* callback, const btVector3& rayFrom, const btVector3& rayTo) const
{
	lockChildShapes();

	btAlignedObjectArray<int> collided;
	btVector3 rayDir(rayTo - rayFrom);
	rayDir.normalize();
	m_box_set.rayQuery(rayDir, rayFrom, collided);

	if (collided.size() == 0)
	{
		unlockChildShapes();
		return;
	}

	int part = (int)getPart();
	btPrimitiveTriangle triangle;
	int i = collided.size();
	while (i--)
	{
		getPrimitiveTriangle(collided[i], triangle);
		callback->processTriangle(triangle.m_vertices, part, collided[i]);
	}
	unlockChildShapes();
}

void btGImpactMeshShapePart::processAllTriangles(btTriangleCallback* callback, const btVector3& aabbMin, const btVector3& aabbMax) const
{
	lockChildShapes();
	btAABB box;
	box.m_min = aabbMin;
	box.m_max = aabbMax;

	btAlignedObjectArray<int> collided;
	m_box_set.boxQuery(box, collided);

	if (collided.size() == 0)
	{
		unlockChildShapes();
		return;
	}

	int part = (int)getPart();
	btPrimitiveTriangle triangle;
	int i = collided.size();
	while (i--)
	{
		this->getPrimitiveTriangle(collided[i], triangle);
		callback->processTriangle(triangle.m_vertices, part, collided[i]);
	}
	unlockChildShapes();
}

void btGImpactMeshShape::processAllTriangles(btTriangleCallback* callback, const btVector3& aabbMin, const btVector3& aabbMax) const
{
	int i = m_mesh_parts.size();
	while (i--)
	{
		m_mesh_parts[i]->processAllTriangles(callback, aabbMin, aabbMax);
	}
}

void btGImpactMeshShape::processAllTrianglesRay(btTriangleCallback* callback, const btVector3& rayFrom, const btVector3& rayTo) const
{
	int i = m_mesh_parts.size();
	while (i--)
	{
		m_mesh_parts[i]->processAllTrianglesRay(callback, rayFrom, rayTo);
	}
}


const char* btGImpactMeshShape::serialize(void* dataBuffer, btSerializer* serializer) const
{
	btGImpactMeshShapeData* trimeshData = (btGImpactMeshShapeData*)dataBuffer;

	btCollisionShape::serialize(&trimeshData->m_collisionShapeData, serializer);

	m_meshInterface->serialize(&trimeshData->m_meshInterface, serializer);

	trimeshData->m_collisionMargin = float(m_collisionMargin);

	localScaling.serializeFloat(trimeshData->m_localScaling);

	trimeshData->m_gimpactSubType = int(getGImpactShapeType());

	return "btGImpactMeshShapeData";
}
