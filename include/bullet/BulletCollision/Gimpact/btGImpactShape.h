


#ifndef GIMPACT_SHAPE_H
#define GIMPACT_SHAPE_H

#include "BulletCollision/CollisionShapes/btCollisionShape.h"
#include "BulletCollision/CollisionShapes/btTriangleShape.h"
#include "BulletCollision/CollisionShapes/btStridingMeshInterface.h"
#include "BulletCollision/CollisionShapes/btCollisionMargin.h"
#include "BulletCollision/CollisionDispatch/btCollisionWorld.h"
#include "BulletCollision/CollisionShapes/btConcaveShape.h"
#include "BulletCollision/CollisionShapes/btTetrahedronShape.h"
#include "LinearMath/btVector3.h"
#include "LinearMath/btTransform.h"
#include "LinearMath/btMatrix3x3.h"
#include "LinearMath/btAlignedObjectArray.h"

#include "btGImpactQuantizedBvh.h"


typedef btGImpactQuantizedBvh btGImpactBoxSet;

enum eGIMPACT_SHAPE_TYPE
{
	CONST_GIMPACT_COMPOUND_SHAPE = 0,
	CONST_GIMPACT_TRIMESH_SHAPE_PART,
	CONST_GIMPACT_TRIMESH_SHAPE
};


class btTetrahedronShapeEx : public btBU_Simplex1to4
{
public:
	btTetrahedronShapeEx()
	{
		m_numVertices = 4;
	}

	SIMD_FORCE_INLINE void setVertices(
		const btVector3& v0, const btVector3& v1,
		const btVector3& v2, const btVector3& v3)
	{
		m_vertices[0] = v0;
		m_vertices[1] = v1;
		m_vertices[2] = v2;
		m_vertices[3] = v3;
		recalcLocalAabb();
	}
};


class btGImpactShapeInterface : public btConcaveShape
{
protected:
	btAABB m_localAABB;
	bool m_needs_update;
	btVector3 localScaling;
	btGImpactBoxSet m_box_set;



	virtual void calcLocalAABB()
	{
		lockChildShapes();
		if (m_box_set.getNodeCount() == 0)
		{
			m_box_set.buildSet();
		}
		else
		{
			m_box_set.update();
		}
		unlockChildShapes();

		m_localAABB = m_box_set.getGlobalBox();
	}

public:
	btGImpactShapeInterface()
	{
		m_shapeType = GIMPACT_SHAPE_PROXYTYPE;
		m_localAABB.invalidate();
		m_needs_update = true;
		localScaling.setValue(1.f, 1.f, 1.f);
	}


	
	SIMD_FORCE_INLINE void updateBound()
	{
		if (!m_needs_update) return;
		calcLocalAABB();
		m_needs_update = false;
	}


	
	void getAabb(const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const
	{
		btAABB transformedbox = m_localAABB;
		transformedbox.appy_transform(t);
		aabbMin = transformedbox.m_min;
		aabbMax = transformedbox.m_max;
	}


	virtual void postUpdate()
	{
		m_needs_update = true;
	}


	SIMD_FORCE_INLINE const btAABB& getLocalBox()
	{
		return m_localAABB;
	}

	virtual int getShapeType() const
	{
		return GIMPACT_SHAPE_PROXYTYPE;
	}

	
	virtual void setLocalScaling(const btVector3& scaling)
	{
		localScaling = scaling;
		postUpdate();
	}

	virtual const btVector3& getLocalScaling() const
	{
		return localScaling;
	}

	virtual void setMargin(btScalar margin)
	{
		m_collisionMargin = margin;
		int i = getNumChildShapes();
		while (i--)
		{
			btCollisionShape* child = getChildShape(i);
			child->setMargin(margin);
		}

		m_needs_update = true;
	}





	virtual eGIMPACT_SHAPE_TYPE getGImpactShapeType() const = 0;


	SIMD_FORCE_INLINE const btGImpactBoxSet* getBoxSet() const
	{
		return &m_box_set;
	}


	SIMD_FORCE_INLINE bool hasBoxSet() const
	{
		if (m_box_set.getNodeCount() == 0) return false;
		return true;
	}


	virtual const btPrimitiveManagerBase* getPrimitiveManager() const = 0;


	virtual int getNumChildShapes() const = 0;


	virtual bool childrenHasTransform() const = 0;


	virtual bool needsRetrieveTriangles() const = 0;


	virtual bool needsRetrieveTetrahedrons() const = 0;

	virtual void getBulletTriangle(int prim_index, btTriangleShapeEx& triangle) const = 0;

	virtual void getBulletTetrahedron(int prim_index, btTetrahedronShapeEx& tetrahedron) const = 0;


	virtual void lockChildShapes() const
	{
	}

	virtual void unlockChildShapes() const
	{
	}


	SIMD_FORCE_INLINE void getPrimitiveTriangle(int index, btPrimitiveTriangle& triangle) const
	{
		getPrimitiveManager()->get_primitive_triangle(index, triangle);
	}


	
	virtual void getChildAabb(int child_index, const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const
	{
		btAABB child_aabb;
		getPrimitiveManager()->get_primitive_box(child_index, child_aabb);
		child_aabb.appy_transform(t);
		aabbMin = child_aabb.m_min;
		aabbMax = child_aabb.m_max;
	}


	virtual btCollisionShape* getChildShape(int index) = 0;


	virtual const btCollisionShape* getChildShape(int index) const = 0;


	virtual btTransform getChildTransform(int index) const = 0;


	
	virtual void setChildTransform(int index, const btTransform& transform) = 0;




	virtual void rayTest(const btVector3& rayFrom, const btVector3& rayTo, btCollisionWorld::RayResultCallback& resultCallback) const
	{
		(void)rayFrom;
		(void)rayTo;
		(void)resultCallback;
	}


	
	virtual void processAllTriangles(btTriangleCallback* callback, const btVector3& aabbMin, const btVector3& aabbMax) const
	{
		(void)callback;
		(void)aabbMin;
		(void)aabbMax;
	}


	
	virtual void processAllTrianglesRay(btTriangleCallback* , const btVector3& , const btVector3& ) const
	{
	}


};



class btGImpactCompoundShape : public btGImpactShapeInterface
{
public:

	class CompoundPrimitiveManager : public btPrimitiveManagerBase
	{
	public:
		virtual ~CompoundPrimitiveManager() {}
		btGImpactCompoundShape* m_compoundShape;

		CompoundPrimitiveManager(const CompoundPrimitiveManager& compound)
			: btPrimitiveManagerBase()
		{
			m_compoundShape = compound.m_compoundShape;
		}

		CompoundPrimitiveManager(btGImpactCompoundShape* compoundShape)
		{
			m_compoundShape = compoundShape;
		}

		CompoundPrimitiveManager()
		{
			m_compoundShape = NULL;
		}

		virtual bool is_trimesh() const
		{
			return false;
		}

		virtual int get_primitive_count() const
		{
			return (int)m_compoundShape->getNumChildShapes();
		}

		virtual void get_primitive_box(int prim_index, btAABB& primbox) const
		{
			btTransform prim_trans;
			if (m_compoundShape->childrenHasTransform())
			{
				prim_trans = m_compoundShape->getChildTransform(prim_index);
			}
			else
			{
				prim_trans.setIdentity();
			}
			const btCollisionShape* shape = m_compoundShape->getChildShape(prim_index);
			shape->getAabb(prim_trans, primbox.m_min, primbox.m_max);
		}

		virtual void get_primitive_triangle(int prim_index, btPrimitiveTriangle& triangle) const
		{
			btAssert(0);
			(void)prim_index;
			(void)triangle;
		}
	};

protected:
	CompoundPrimitiveManager m_primitive_manager;
	btAlignedObjectArray<btTransform> m_childTransforms;
	btAlignedObjectArray<btCollisionShape*> m_childShapes;

public:
	btGImpactCompoundShape(bool children_has_transform = true)
	{
		(void)children_has_transform;
		m_primitive_manager.m_compoundShape = this;
		m_box_set.setPrimitiveManager(&m_primitive_manager);
	}

	virtual ~btGImpactCompoundShape()
	{
	}


	virtual bool childrenHasTransform() const
	{
		if (m_childTransforms.size() == 0) return false;
		return true;
	}


	virtual const btPrimitiveManagerBase* getPrimitiveManager() const
	{
		return &m_primitive_manager;
	}


	SIMD_FORCE_INLINE CompoundPrimitiveManager* getCompoundPrimitiveManager()
	{
		return &m_primitive_manager;
	}


	virtual int getNumChildShapes() const
	{
		return m_childShapes.size();
	}


	void addChildShape(const btTransform& localTransform, btCollisionShape* shape)
	{
		btAssert(shape->isConvex());
		m_childTransforms.push_back(localTransform);
		m_childShapes.push_back(shape);
	}


	void addChildShape(btCollisionShape* shape)
	{
		btAssert(shape->isConvex());
		m_childShapes.push_back(shape);
	}


	virtual btCollisionShape* getChildShape(int index)
	{
		return m_childShapes[index];
	}


	virtual const btCollisionShape* getChildShape(int index) const
	{
		return m_childShapes[index];
	}


	
	virtual void getChildAabb(int child_index, const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const
	{
		if (childrenHasTransform())
		{
			m_childShapes[child_index]->getAabb(t * m_childTransforms[child_index], aabbMin, aabbMax);
		}
		else
		{
			m_childShapes[child_index]->getAabb(t, aabbMin, aabbMax);
		}
	}


	virtual btTransform getChildTransform(int index) const
	{
		btAssert(m_childTransforms.size() == m_childShapes.size());
		return m_childTransforms[index];
	}


	
	virtual void setChildTransform(int index, const btTransform& transform)
	{
		btAssert(m_childTransforms.size() == m_childShapes.size());
		m_childTransforms[index] = transform;
		postUpdate();
	}


	virtual bool needsRetrieveTriangles() const
	{
		return false;
	}


	virtual bool needsRetrieveTetrahedrons() const
	{
		return false;
	}

	virtual void getBulletTriangle(int prim_index, btTriangleShapeEx& triangle) const
	{
		(void)prim_index;
		(void)triangle;
		btAssert(0);
	}

	virtual void getBulletTetrahedron(int prim_index, btTetrahedronShapeEx& tetrahedron) const
	{
		(void)prim_index;
		(void)tetrahedron;
		btAssert(0);
	}


	virtual void calculateLocalInertia(btScalar mass, btVector3& inertia) const;

	virtual const char* getName() const
	{
		return "GImpactCompound";
	}

	virtual eGIMPACT_SHAPE_TYPE getGImpactShapeType() const
	{
		return CONST_GIMPACT_COMPOUND_SHAPE;
	}
};



class btGImpactMeshShapePart : public btGImpactShapeInterface
{
public:

	
	class TrimeshPrimitiveManager : public btPrimitiveManagerBase
	{
	public:
		btScalar m_margin;
		btStridingMeshInterface* m_meshInterface;
		btVector3 m_scale;
		int m_part;
		int m_lock_count;
		const unsigned char* vertexbase;
		int numverts;
		PHY_ScalarType type;
		int stride;
		const unsigned char* indexbase;
		int indexstride;
		int numfaces;
		PHY_ScalarType indicestype;

		TrimeshPrimitiveManager()
		{
			m_meshInterface = NULL;
			m_part = 0;
			m_margin = 0.01f;
			m_scale = btVector3(1.f, 1.f, 1.f);
			m_lock_count = 0;
			vertexbase = 0;
			numverts = 0;
			stride = 0;
			indexbase = 0;
			indexstride = 0;
			numfaces = 0;
		}

		TrimeshPrimitiveManager(const TrimeshPrimitiveManager& manager)
			: btPrimitiveManagerBase()
		{
			m_meshInterface = manager.m_meshInterface;
			m_part = manager.m_part;
			m_margin = manager.m_margin;
			m_scale = manager.m_scale;
			m_lock_count = 0;
			vertexbase = 0;
			numverts = 0;
			stride = 0;
			indexbase = 0;
			indexstride = 0;
			numfaces = 0;
		}

		TrimeshPrimitiveManager(
			btStridingMeshInterface* meshInterface, int part)
		{
			m_meshInterface = meshInterface;
			m_part = part;
			m_scale = m_meshInterface->getScaling();
			m_margin = 0.1f;
			m_lock_count = 0;
			vertexbase = 0;
			numverts = 0;
			stride = 0;
			indexbase = 0;
			indexstride = 0;
			numfaces = 0;
		}

		virtual ~TrimeshPrimitiveManager() {}

		void lock()
		{
			if (m_lock_count > 0)
			{
				m_lock_count++;
				return;
			}
			m_meshInterface->getLockedReadOnlyVertexIndexBase(
				&vertexbase, numverts,
				type, stride, &indexbase, indexstride, numfaces, indicestype, m_part);

			m_lock_count = 1;
		}

		void unlock()
		{
			if (m_lock_count == 0) return;
			if (m_lock_count > 1)
			{
				--m_lock_count;
				return;
			}
			m_meshInterface->unLockReadOnlyVertexBase(m_part);
			vertexbase = NULL;
			m_lock_count = 0;
		}

		virtual bool is_trimesh() const
		{
			return true;
		}

		virtual int get_primitive_count() const
		{
			return (int)numfaces;
		}

		SIMD_FORCE_INLINE int get_vertex_count() const
		{
			return (int)numverts;
		}

		SIMD_FORCE_INLINE void get_indices(int face_index, unsigned int& i0, unsigned int& i1, unsigned int& i2) const
		{
			if (indicestype == PHY_SHORT)
			{
				unsigned short* s_indices = (unsigned short*)(indexbase + face_index * indexstride);
				i0 = s_indices[0];
				i1 = s_indices[1];
				i2 = s_indices[2];
			}
			else if (indicestype == PHY_INTEGER)
			{
				unsigned int* i_indices = (unsigned int*)(indexbase + face_index * indexstride);
				i0 = i_indices[0];
				i1 = i_indices[1];
				i2 = i_indices[2];
			}
			else
			{
				btAssert(indicestype == PHY_UCHAR);
				unsigned char* i_indices = (unsigned char*)(indexbase + face_index * indexstride);
				i0 = i_indices[0];
				i1 = i_indices[1];
				i2 = i_indices[2];
			}
		}

		SIMD_FORCE_INLINE void get_vertex(unsigned int vertex_index, btVector3& Vertex) const
		{
			if (type == PHY_DOUBLE)
			{
				double* dvertices = (double*)(vertexbase + vertex_index * stride);
				Vertex[0] = btScalar(dvertices[0] * m_scale[0]);
				Vertex[1] = btScalar(dvertices[1] * m_scale[1]);
				Vertex[2] = btScalar(dvertices[2] * m_scale[2]);
			}
			else
			{
				float* svertices = (float*)(vertexbase + vertex_index * stride);
				Vertex[0] = svertices[0] * m_scale[0];
				Vertex[1] = svertices[1] * m_scale[1];
				Vertex[2] = svertices[2] * m_scale[2];
			}
		}

		virtual void get_primitive_box(int prim_index, btAABB& primbox) const
		{
			btPrimitiveTriangle triangle;
			get_primitive_triangle(prim_index, triangle);
			primbox.calc_from_triangle_margin(
				triangle.m_vertices[0],
				triangle.m_vertices[1], triangle.m_vertices[2], triangle.m_margin);
		}

		virtual void get_primitive_triangle(int prim_index, btPrimitiveTriangle& triangle) const
		{
			unsigned int indices[3];
			get_indices(prim_index, indices[0], indices[1], indices[2]);
			get_vertex(indices[0], triangle.m_vertices[0]);
			get_vertex(indices[1], triangle.m_vertices[1]);
			get_vertex(indices[2], triangle.m_vertices[2]);
			triangle.m_margin = m_margin;
		}

		SIMD_FORCE_INLINE void get_bullet_triangle(int prim_index, btTriangleShapeEx& triangle) const
		{
			unsigned int indices[3];
			get_indices(prim_index, indices[0], indices[1], indices[2]);
			get_vertex(indices[0], triangle.m_vertices1[0]);
			get_vertex(indices[1], triangle.m_vertices1[1]);
			get_vertex(indices[2], triangle.m_vertices1[2]);
			triangle.setMargin(m_margin);
		}
	};

protected:
	TrimeshPrimitiveManager m_primitive_manager;

public:
	btGImpactMeshShapePart()
	{
		m_box_set.setPrimitiveManager(&m_primitive_manager);
	}

	btGImpactMeshShapePart(btStridingMeshInterface* meshInterface, int part);
	virtual ~btGImpactMeshShapePart();


	virtual bool childrenHasTransform() const
	{
		return false;
	}


	virtual void lockChildShapes() const;
	virtual void unlockChildShapes() const;


	virtual int getNumChildShapes() const
	{
		return m_primitive_manager.get_primitive_count();
	}


	virtual btCollisionShape* getChildShape(int index)
	{
		(void)index;
		btAssert(0);
		return NULL;
	}


	virtual const btCollisionShape* getChildShape(int index) const
	{
		(void)index;
		btAssert(0);
		return NULL;
	}


	virtual btTransform getChildTransform(int index) const
	{
		(void)index;
		btAssert(0);
		return btTransform();
	}


	
	virtual void setChildTransform(int index, const btTransform& transform)
	{
		(void)index;
		(void)transform;
		btAssert(0);
	}


	virtual const btPrimitiveManagerBase* getPrimitiveManager() const
	{
		return &m_primitive_manager;
	}

	SIMD_FORCE_INLINE TrimeshPrimitiveManager* getTrimeshPrimitiveManager()
	{
		return &m_primitive_manager;
	}

	virtual void calculateLocalInertia(btScalar mass, btVector3& inertia) const;

	virtual const char* getName() const
	{
		return "GImpactMeshShapePart";
	}

	virtual eGIMPACT_SHAPE_TYPE getGImpactShapeType() const
	{
		return CONST_GIMPACT_TRIMESH_SHAPE_PART;
	}


	virtual bool needsRetrieveTriangles() const
	{
		return true;
	}


	virtual bool needsRetrieveTetrahedrons() const
	{
		return false;
	}

	virtual void getBulletTriangle(int prim_index, btTriangleShapeEx& triangle) const
	{
		m_primitive_manager.get_bullet_triangle(prim_index, triangle);
	}

	virtual void getBulletTetrahedron(int prim_index, btTetrahedronShapeEx& tetrahedron) const
	{
		(void)prim_index;
		(void)tetrahedron;
		btAssert(0);
	}

	SIMD_FORCE_INLINE int getVertexCount() const
	{
		return m_primitive_manager.get_vertex_count();
	}

	SIMD_FORCE_INLINE void getVertex(int vertex_index, btVector3& Vertex) const
	{
		m_primitive_manager.get_vertex(vertex_index, Vertex);
	}

	SIMD_FORCE_INLINE void setMargin(btScalar margin)
	{
		m_primitive_manager.m_margin = margin;
		postUpdate();
	}

	SIMD_FORCE_INLINE btScalar getMargin() const
	{
		return m_primitive_manager.m_margin;
	}

	virtual void setLocalScaling(const btVector3& scaling)
	{
		m_primitive_manager.m_scale = scaling;
		postUpdate();
	}

	virtual const btVector3& getLocalScaling() const
	{
		return m_primitive_manager.m_scale;
	}

	SIMD_FORCE_INLINE int getPart() const
	{
		return (int)m_primitive_manager.m_part;
	}

	virtual void processAllTriangles(btTriangleCallback* callback, const btVector3& aabbMin, const btVector3& aabbMax) const;
	virtual void processAllTrianglesRay(btTriangleCallback* callback, const btVector3& rayFrom, const btVector3& rayTo) const;
};



class btGImpactMeshShape : public btGImpactShapeInterface
{
	btStridingMeshInterface* m_meshInterface;

protected:
	btAlignedObjectArray<btGImpactMeshShapePart*> m_mesh_parts;
	void buildMeshParts(btStridingMeshInterface* meshInterface)
	{
		for (int i = 0; i < meshInterface->getNumSubParts(); ++i)
		{
			btGImpactMeshShapePart* newpart = new btGImpactMeshShapePart(meshInterface, i);
			m_mesh_parts.push_back(newpart);
		}
	}


	virtual void calcLocalAABB()
	{
		m_localAABB.invalidate();
		int i = m_mesh_parts.size();
		while (i--)
		{
			m_mesh_parts[i]->updateBound();
			m_localAABB.merge(m_mesh_parts[i]->getLocalBox());
		}
	}

public:
	btGImpactMeshShape(btStridingMeshInterface* meshInterface)
	{
		m_meshInterface = meshInterface;
		buildMeshParts(meshInterface);
	}

	virtual ~btGImpactMeshShape()
	{
		int i = m_mesh_parts.size();
		while (i--)
		{
			btGImpactMeshShapePart* part = m_mesh_parts[i];
			delete part;
		}
		m_mesh_parts.clear();
	}

	btStridingMeshInterface* getMeshInterface()
	{
		return m_meshInterface;
	}

	const btStridingMeshInterface* getMeshInterface() const
	{
		return m_meshInterface;
	}

	int getMeshPartCount() const
	{
		return m_mesh_parts.size();
	}

	btGImpactMeshShapePart* getMeshPart(int index)
	{
		return m_mesh_parts[index];
	}

	const btGImpactMeshShapePart* getMeshPart(int index) const
	{
		return m_mesh_parts[index];
	}

	virtual void setLocalScaling(const btVector3& scaling)
	{
		localScaling = scaling;

		int i = m_mesh_parts.size();
		while (i--)
		{
			btGImpactMeshShapePart* part = m_mesh_parts[i];
			part->setLocalScaling(scaling);
		}

		m_needs_update = true;
	}

	virtual void setMargin(btScalar margin)
	{
		m_collisionMargin = margin;

		int i = m_mesh_parts.size();
		while (i--)
		{
			btGImpactMeshShapePart* part = m_mesh_parts[i];
			part->setMargin(margin);
		}

		m_needs_update = true;
	}


	virtual void postUpdate()
	{
		int i = m_mesh_parts.size();
		while (i--)
		{
			btGImpactMeshShapePart* part = m_mesh_parts[i];
			part->postUpdate();
		}

		m_needs_update = true;
	}

	virtual void calculateLocalInertia(btScalar mass, btVector3& inertia) const;


	virtual const btPrimitiveManagerBase* getPrimitiveManager() const
	{
		btAssert(0);
		return NULL;
	}


	virtual int getNumChildShapes() const
	{
		btAssert(0);
		return 0;
	}


	virtual bool childrenHasTransform() const
	{
		btAssert(0);
		return false;
	}


	virtual bool needsRetrieveTriangles() const
	{
		btAssert(0);
		return false;
	}


	virtual bool needsRetrieveTetrahedrons() const
	{
		btAssert(0);
		return false;
	}

	virtual void getBulletTriangle(int prim_index, btTriangleShapeEx& triangle) const
	{
		(void)prim_index;
		(void)triangle;
		btAssert(0);
	}

	virtual void getBulletTetrahedron(int prim_index, btTetrahedronShapeEx& tetrahedron) const
	{
		(void)prim_index;
		(void)tetrahedron;
		btAssert(0);
	}


	virtual void lockChildShapes() const
	{
		btAssert(0);
	}

	virtual void unlockChildShapes() const
	{
		btAssert(0);
	}


	
	virtual void getChildAabb(int child_index, const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const
	{
		(void)child_index;
		(void)t;
		(void)aabbMin;
		(void)aabbMax;
		btAssert(0);
	}


	virtual btCollisionShape* getChildShape(int index)
	{
		(void)index;
		btAssert(0);
		return NULL;
	}


	virtual const btCollisionShape* getChildShape(int index) const
	{
		(void)index;
		btAssert(0);
		return NULL;
	}


	virtual btTransform getChildTransform(int index) const
	{
		(void)index;
		btAssert(0);
		return btTransform();
	}


	
	virtual void setChildTransform(int index, const btTransform& transform)
	{
		(void)index;
		(void)transform;
		btAssert(0);
	}

	virtual eGIMPACT_SHAPE_TYPE getGImpactShapeType() const
	{
		return CONST_GIMPACT_TRIMESH_SHAPE;
	}

	virtual const char* getName() const
	{
		return "GImpactMesh";
	}

	virtual void rayTest(const btVector3& rayFrom, const btVector3& rayTo, btCollisionWorld::RayResultCallback& resultCallback) const;


	
	virtual void processAllTriangles(btTriangleCallback* callback, const btVector3& aabbMin, const btVector3& aabbMax) const;

	virtual void processAllTrianglesRay(btTriangleCallback* callback, const btVector3& rayFrom, const btVector3& rayTo) const;

	virtual int calculateSerializeBufferSize() const;


	virtual const char* serialize(void* dataBuffer, btSerializer* serializer) const;
};


struct btGImpactMeshShapeData
{
	btCollisionShapeData m_collisionShapeData;

	btStridingMeshInterfaceData m_meshInterface;

	btVector3FloatData m_localScaling;

	float m_collisionMargin;

	int m_gimpactSubType;
};

SIMD_FORCE_INLINE int btGImpactMeshShape::calculateSerializeBufferSize() const
{
	return sizeof(btGImpactMeshShapeData);
}

#endif
