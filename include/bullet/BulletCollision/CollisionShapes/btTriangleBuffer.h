

#ifndef BT_TRIANGLE_BUFFER_H
#define BT_TRIANGLE_BUFFER_H

#include "btTriangleCallback.h"
#include "LinearMath/btAlignedObjectArray.h"

struct btTriangle
{
	btVector3 m_vertex0;
	btVector3 m_vertex1;
	btVector3 m_vertex2;
	int m_partId;
	int m_triangleIndex;
};










class btTriangleBuffer : public btTriangleCallback
{
	btAlignedObjectArray<btTriangle> m_triangleBuffer;

public:
	virtual void processTriangle(btVector3* triangle, int partId, int triangleIndex);

	int getNumTriangles() const
	{
		return int(m_triangleBuffer.size());
	}

	const btTriangle& getTriangle(int index) const
	{
		return m_triangleBuffer[index];
	}

	void clearBuffer()
	{
		m_triangleBuffer.clear();
	}
};

#endif  
