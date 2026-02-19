

#ifndef BT_SIMPLEX_1TO4_SHAPE
#define BT_SIMPLEX_1TO4_SHAPE

#include "btPolyhedralConvexShape.h"
#include "BulletCollision/BroadphaseCollision/btBroadphaseProxy.h"


ATTRIBUTE_ALIGNED16(class)
btBU_Simplex1to4 : public btPolyhedralConvexAabbCachingShape
{
protected:
	int m_numVertices;
	btVector3 m_vertices[4];

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btBU_Simplex1to4();

	btBU_Simplex1to4(const btVector3& pt0);
	btBU_Simplex1to4(const btVector3& pt0, const btVector3& pt1);
	btBU_Simplex1to4(const btVector3& pt0, const btVector3& pt1, const btVector3& pt2);
	btBU_Simplex1to4(const btVector3& pt0, const btVector3& pt1, const btVector3& pt2, const btVector3& pt3);

	void reset()
	{
		m_numVertices = 0;
	}

	virtual void getAabb(const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const;

	void addVertex(const btVector3& pt);

	

	virtual int getNumVertices() const;

	virtual int getNumEdges() const;

	virtual void getEdge(int i, btVector3& pa, btVector3& pb) const;

	virtual void getVertex(int i, btVector3& vtx) const;

	virtual int getNumPlanes() const;

	virtual void getPlane(btVector3 & planeNormal, btVector3 & planeSupport, int i) const;

	virtual int getIndex(int i) const;

	virtual bool isInside(const btVector3& pt, btScalar tolerance) const;

	
	virtual const char* getName() const { return "btBU_Simplex1to4"; }
};

#endif  
