

#ifndef BT_TRIANGLE_MESH_H
#define BT_TRIANGLE_MESH_H

#include "btTriangleIndexVertexArray.h"
#include "LinearMath/btVector3.h"
#include "LinearMath/btAlignedObjectArray.h"





class btTriangleMesh : public btTriangleIndexVertexArray
{
	btAlignedObjectArray<btVector3> m_4componentVertices;
	btAlignedObjectArray<btScalar> m_3componentVertices;

	btAlignedObjectArray<unsigned int> m_32bitIndices;
	btAlignedObjectArray<unsigned short int> m_16bitIndices;
	bool m_use32bitIndices;
	bool m_use4componentVertices;

public:
	btScalar m_weldingThreshold;

	btTriangleMesh(bool use32bitIndices = true, bool use4componentVertices = true);

	bool getUse32bitIndices() const
	{
		return m_use32bitIndices;
	}

	bool getUse4componentVertices() const
	{
		return m_use4componentVertices;
	}
	
	
	void addTriangle(const btVector3& vertex0, const btVector3& vertex1, const btVector3& vertex2, bool removeDuplicateVertices = false);

	
	void addTriangleIndices(int index1, int index2, int index3);

	int getNumTriangles() const;

	virtual void preallocateVertices(int numverts);
	virtual void preallocateIndices(int numindices);

	
	int findOrAddVertex(const btVector3& vertex, bool removeDuplicateVertices);
	
	void addIndex(int index);
};

#endif  
