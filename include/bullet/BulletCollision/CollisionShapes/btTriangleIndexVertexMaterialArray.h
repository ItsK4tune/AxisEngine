



#ifndef BT_MULTIMATERIAL_TRIANGLE_INDEX_VERTEX_ARRAY_H
#define BT_MULTIMATERIAL_TRIANGLE_INDEX_VERTEX_ARRAY_H

#include "btTriangleIndexVertexArray.h"

ATTRIBUTE_ALIGNED16(struct)
btMaterialProperties
{
	
	int m_numMaterials;
	const unsigned char* m_materialBase;
	int m_materialStride;
	PHY_ScalarType m_materialType;
	
	
	
	
	int m_numTriangles;
	const unsigned char* m_triangleMaterialsBase;
	int m_triangleMaterialStride;
	
	PHY_ScalarType m_triangleType;
};

typedef btAlignedObjectArray<btMaterialProperties> MaterialArray;







ATTRIBUTE_ALIGNED16(class)
btTriangleIndexVertexMaterialArray : public btTriangleIndexVertexArray
{
protected:
	MaterialArray m_materials;

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btTriangleIndexVertexMaterialArray()
	{
	}

	btTriangleIndexVertexMaterialArray(int numTriangles, int* triangleIndexBase, int triangleIndexStride,
									   int numVertices, btScalar* vertexBase, int vertexStride,
									   int numMaterials, unsigned char* materialBase, int materialStride,
									   int* triangleMaterialsBase, int materialIndexStride);

	virtual ~btTriangleIndexVertexMaterialArray() {}

	void addMaterialProperties(const btMaterialProperties& mat, PHY_ScalarType triangleType = PHY_INTEGER)
	{
		m_materials.push_back(mat);
		m_materials[m_materials.size() - 1].m_triangleType = triangleType;
	}

	virtual void getLockedMaterialBase(unsigned char** materialBase, int& numMaterials, PHY_ScalarType& materialType, int& materialStride,
									   unsigned char** triangleMaterialBase, int& numTriangles, int& triangleMaterialStride, PHY_ScalarType& triangleType, int subpart = 0);

	virtual void getLockedReadOnlyMaterialBase(const unsigned char** materialBase, int& numMaterials, PHY_ScalarType& materialType, int& materialStride,
											   const unsigned char** triangleMaterialBase, int& numTriangles, int& triangleMaterialStride, PHY_ScalarType& triangleType, int subpart = 0);
};

#endif  
