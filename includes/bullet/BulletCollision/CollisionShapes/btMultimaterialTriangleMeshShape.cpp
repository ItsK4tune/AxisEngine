



#include "BulletCollision/CollisionShapes/btMultimaterialTriangleMeshShape.h"
#include "BulletCollision/CollisionShapes/btTriangleIndexVertexMaterialArray.h"



const btMaterial *btMultimaterialTriangleMeshShape::getMaterialProperties(int partID, int triIndex)
{
	const unsigned char *materialBase = 0;
	int numMaterials;
	PHY_ScalarType materialType;
	int materialStride;
	const unsigned char *triangleMaterialBase = 0;
	int numTriangles;
	int triangleMaterialStride;
	PHY_ScalarType triangleType;

	((btTriangleIndexVertexMaterialArray *)m_meshInterface)->getLockedReadOnlyMaterialBase(&materialBase, numMaterials, materialType, materialStride, &triangleMaterialBase, numTriangles, triangleMaterialStride, triangleType, partID);

	
	
	
	
	int *matInd = (int *)(&(triangleMaterialBase[(triIndex * triangleMaterialStride)]));
	btMaterial *matVal = (btMaterial *)(&(materialBase[*matInd * materialStride]));
	return (matVal);
}
