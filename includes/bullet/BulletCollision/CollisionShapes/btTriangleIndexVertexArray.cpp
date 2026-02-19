

#include "btTriangleIndexVertexArray.h"

btTriangleIndexVertexArray::btTriangleIndexVertexArray(int numTriangles, int* triangleIndexBase, int triangleIndexStride, int numVertices, btScalar* vertexBase, int vertexStride)
	: m_hasAabb(0)
{
	btIndexedMesh mesh;

	mesh.m_numTriangles = numTriangles;
	mesh.m_triangleIndexBase = (const unsigned char*)triangleIndexBase;
	mesh.m_triangleIndexStride = triangleIndexStride;
	mesh.m_numVertices = numVertices;
	mesh.m_vertexBase = (const unsigned char*)vertexBase;
	mesh.m_vertexStride = vertexStride;

	addIndexedMesh(mesh);
}

btTriangleIndexVertexArray::~btTriangleIndexVertexArray()
{
}

void btTriangleIndexVertexArray::getLockedVertexIndexBase(unsigned char** vertexbase, int& numverts, PHY_ScalarType& type, int& vertexStride, unsigned char** indexbase, int& indexstride, int& numfaces, PHY_ScalarType& indicestype, int subpart)
{
	btAssert(subpart < getNumSubParts());

	btIndexedMesh& mesh = m_indexedMeshes[subpart];

	numverts = mesh.m_numVertices;
	(*vertexbase) = (unsigned char*)mesh.m_vertexBase;

	type = mesh.m_vertexType;

	vertexStride = mesh.m_vertexStride;

	numfaces = mesh.m_numTriangles;

	(*indexbase) = (unsigned char*)mesh.m_triangleIndexBase;
	indexstride = mesh.m_triangleIndexStride;
	indicestype = mesh.m_indexType;
}

void btTriangleIndexVertexArray::getLockedReadOnlyVertexIndexBase(const unsigned char** vertexbase, int& numverts, PHY_ScalarType& type, int& vertexStride, const unsigned char** indexbase, int& indexstride, int& numfaces, PHY_ScalarType& indicestype, int subpart) const
{
	const btIndexedMesh& mesh = m_indexedMeshes[subpart];

	numverts = mesh.m_numVertices;
	(*vertexbase) = (const unsigned char*)mesh.m_vertexBase;

	type = mesh.m_vertexType;

	vertexStride = mesh.m_vertexStride;

	numfaces = mesh.m_numTriangles;
	(*indexbase) = (const unsigned char*)mesh.m_triangleIndexBase;
	indexstride = mesh.m_triangleIndexStride;
	indicestype = mesh.m_indexType;
}

bool btTriangleIndexVertexArray::hasPremadeAabb() const
{
	return (m_hasAabb == 1);
}

void btTriangleIndexVertexArray::setPremadeAabb(const btVector3& aabbMin, const btVector3& aabbMax) const
{
	m_aabbMin = aabbMin;
	m_aabbMax = aabbMax;
	m_hasAabb = 1;  
}

void btTriangleIndexVertexArray::getPremadeAabb(btVector3* aabbMin, btVector3* aabbMax) const
{
	*aabbMin = m_aabbMin;
	*aabbMax = m_aabbMax;
}
