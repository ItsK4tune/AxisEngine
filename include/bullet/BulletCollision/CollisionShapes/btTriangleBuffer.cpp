

#include "btTriangleBuffer.h"

void btTriangleBuffer::processTriangle(btVector3* triangle, int partId, int triangleIndex)
{
	btTriangle tri;
	tri.m_vertex0 = triangle[0];
	tri.m_vertex1 = triangle[1];
	tri.m_vertex2 = triangle[2];
	tri.m_partId = partId;
	tri.m_triangleIndex = triangleIndex;

	m_triangleBuffer.push_back(tri);
}
