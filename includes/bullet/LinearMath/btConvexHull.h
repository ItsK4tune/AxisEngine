




#ifndef BT_CD_HULL_H
#define BT_CD_HULL_H

#include "btVector3.h"
#include "btAlignedObjectArray.h"

typedef btAlignedObjectArray<unsigned int> TUIntArray;

class HullResult
{
public:
	HullResult(void)
	{
		mPolygons = true;
		mNumOutputVertices = 0;
		mNumFaces = 0;
		mNumIndices = 0;
	}
	bool mPolygons;                                    
	unsigned int mNumOutputVertices;                   
	btAlignedObjectArray<btVector3> m_OutputVertices;  
	unsigned int mNumFaces;                            
	unsigned int mNumIndices;                          
	btAlignedObjectArray<unsigned int> m_Indices;      

	
	
};

enum HullFlag
{
	QF_TRIANGLES = (1 << 0),      
	QF_REVERSE_ORDER = (1 << 1),  
	QF_DEFAULT = QF_TRIANGLES
};

class HullDesc
{
public:
	HullDesc(void)
	{
		mFlags = QF_DEFAULT;
		mVcount = 0;
		mVertices = 0;
		mVertexStride = sizeof(btVector3);
		mNormalEpsilon = 0.001f;
		mMaxVertices = 4096;  
		mMaxFaces = 4096;
	};

	HullDesc(HullFlag flag,
			 unsigned int vcount,
			 const btVector3* vertices,
			 unsigned int stride = sizeof(btVector3))
	{
		mFlags = flag;
		mVcount = vcount;
		mVertices = vertices;
		mVertexStride = stride;
		mNormalEpsilon = btScalar(0.001);
		mMaxVertices = 4096;
	}

	bool HasHullFlag(HullFlag flag) const
	{
		if (mFlags & flag) return true;
		return false;
	}

	void SetHullFlag(HullFlag flag)
	{
		mFlags |= flag;
	}

	void ClearHullFlag(HullFlag flag)
	{
		mFlags &= ~flag;
	}

	unsigned int mFlags;         
	unsigned int mVcount;        
	const btVector3* mVertices;  
	unsigned int mVertexStride;  
	btScalar mNormalEpsilon;     
	unsigned int mMaxVertices;   
	unsigned int mMaxFaces;
};

enum HullError
{
	QE_OK,   
	QE_FAIL  
};

class btPlane
{
public:
	btVector3 normal;
	btScalar dist;  
	btPlane(const btVector3& n, btScalar d) : normal(n), dist(d) {}
	btPlane() : normal(), dist(0) {}
};

class ConvexH
{
public:
	class HalfEdge
	{
	public:
		short ea;         
		unsigned char v;  
		unsigned char p;  
		HalfEdge() {}
		HalfEdge(short _ea, unsigned char _v, unsigned char _p) : ea(_ea), v(_v), p(_p) {}
	};
	ConvexH()
	{
	}
	~ConvexH()
	{
	}
	btAlignedObjectArray<btVector3> vertices;
	btAlignedObjectArray<HalfEdge> edges;
	btAlignedObjectArray<btPlane> facets;
	ConvexH(int vertices_size, int edges_size, int facets_size);
};

class int4
{
public:
	int x, y, z, w;
	int4(){};
	int4(int _x, int _y, int _z, int _w)
	{
		x = _x;
		y = _y;
		z = _z;
		w = _w;
	}
	const int& operator[](int i) const { return (&x)[i]; }
	int& operator[](int i) { return (&x)[i]; }
};

class PHullResult
{
public:
	PHullResult(void)
	{
		mVcount = 0;
		mIndexCount = 0;
		mFaceCount = 0;
		mVertices = 0;
	}

	unsigned int mVcount;
	unsigned int mIndexCount;
	unsigned int mFaceCount;
	btVector3* mVertices;
	TUIntArray m_Indices;
};



class HullLibrary
{
	btAlignedObjectArray<class btHullTriangle*> m_tris;

public:
	btAlignedObjectArray<int> m_vertexIndexMapping;

	HullError CreateConvexHull(const HullDesc& desc,  
							   HullResult& result);   
	HullError ReleaseResult(HullResult& result);      

private:
	bool ComputeHull(unsigned int vcount, const btVector3* vertices, PHullResult& result, unsigned int vlimit);

	class btHullTriangle* allocateTriangle(int a, int b, int c);
	void deAllocateTriangle(btHullTriangle*);
	void b2bfix(btHullTriangle* s, btHullTriangle* t);

	void removeb2b(btHullTriangle* s, btHullTriangle* t);

	void checkit(btHullTriangle* t);

	btHullTriangle* extrudable(btScalar epsilon);

	int calchull(btVector3* verts, int verts_count, TUIntArray& tris_out, int& tris_count, int vlimit);

	int calchullgen(btVector3* verts, int verts_count, int vlimit);

	int4 FindSimplex(btVector3* verts, int verts_count, btAlignedObjectArray<int>& allow);

	class ConvexH* ConvexHCrop(ConvexH& convex, const btPlane& slice);

	void extrude(class btHullTriangle* t0, int v);

	ConvexH* test_cube();

	
	
	
	
	void BringOutYourDead(const btVector3* verts, unsigned int vcount, btVector3* overts, unsigned int& ocount, unsigned int* indices, unsigned indexcount);

	bool CleanupVertices(unsigned int svcount,
						 const btVector3* svertices,
						 unsigned int stride,
						 unsigned int& vcount,  
						 btVector3* vertices,   
						 btScalar normalepsilon,
						 btVector3& scale);
};

#endif  
