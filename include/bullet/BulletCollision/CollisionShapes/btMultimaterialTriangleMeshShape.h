



#ifndef BT_BVH_TRIANGLE_MATERIAL_MESH_SHAPE_H
#define BT_BVH_TRIANGLE_MATERIAL_MESH_SHAPE_H

#include "btBvhTriangleMeshShape.h"
#include "btMaterial.h"


ATTRIBUTE_ALIGNED16(class)
btMultimaterialTriangleMeshShape : public btBvhTriangleMeshShape
{
	btAlignedObjectArray<btMaterial *> m_materialList;

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btMultimaterialTriangleMeshShape(btStridingMeshInterface * meshInterface, bool useQuantizedAabbCompression, bool buildBvh = true) : btBvhTriangleMeshShape(meshInterface, useQuantizedAabbCompression, buildBvh)
	{
		m_shapeType = MULTIMATERIAL_TRIANGLE_MESH_PROXYTYPE;

		const unsigned char *vertexbase;
		int numverts;
		PHY_ScalarType type;
		int stride;
		const unsigned char *indexbase;
		int indexstride;
		int numfaces;
		PHY_ScalarType indicestype;

		

		for (int i = 0; i < meshInterface->getNumSubParts(); i++)
		{
			m_meshInterface->getLockedReadOnlyVertexIndexBase(
				&vertexbase,
				numverts,
				type,
				stride,
				&indexbase,
				indexstride,
				numfaces,
				indicestype,
				i);
			
		}
	}

	
	btMultimaterialTriangleMeshShape(btStridingMeshInterface * meshInterface, bool useQuantizedAabbCompression, const btVector3 &bvhAabbMin, const btVector3 &bvhAabbMax, bool buildBvh = true) : btBvhTriangleMeshShape(meshInterface, useQuantizedAabbCompression, bvhAabbMin, bvhAabbMax, buildBvh)
	{
		m_shapeType = MULTIMATERIAL_TRIANGLE_MESH_PROXYTYPE;

		const unsigned char *vertexbase;
		int numverts;
		PHY_ScalarType type;
		int stride;
		const unsigned char *indexbase;
		int indexstride;
		int numfaces;
		PHY_ScalarType indicestype;

		

		for (int i = 0; i < meshInterface->getNumSubParts(); i++)
		{
			m_meshInterface->getLockedReadOnlyVertexIndexBase(
				&vertexbase,
				numverts,
				type,
				stride,
				&indexbase,
				indexstride,
				numfaces,
				indicestype,
				i);
			
		}
	}

	virtual ~btMultimaterialTriangleMeshShape()
	{
		
	}
	
	virtual const char *getName() const { return "MULTIMATERIALTRIANGLEMESH"; }

	
	const btMaterial *getMaterialProperties(int partID, int triIndex);
};

#endif  
