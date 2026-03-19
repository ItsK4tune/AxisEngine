

#ifndef BT_STRIDING_MESHINTERFACE_H
#define BT_STRIDING_MESHINTERFACE_H

#include "LinearMath/btVector3.h"
#include "btTriangleCallback.h"
#include "btConcaveShape.h"




ATTRIBUTE_ALIGNED16(class)
btStridingMeshInterface
{
protected:
	btVector3 m_scaling;

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btStridingMeshInterface() : m_scaling(btScalar(1.), btScalar(1.), btScalar(1.))
	{
	}

	virtual ~btStridingMeshInterface();

	virtual void InternalProcessAllTriangles(btInternalTriangleIndexCallback * callback, const btVector3& aabbMin, const btVector3& aabbMax) const;

	
	void calculateAabbBruteForce(btVector3 & aabbMin, btVector3 & aabbMax);

	
	
	
	
	
	virtual void getLockedVertexIndexBase(unsigned char** vertexbase, int& numverts, PHY_ScalarType& type, int& stride, unsigned char** indexbase, int& indexstride, int& numfaces, PHY_ScalarType& indicestype, int subpart = 0) = 0;

	virtual void getLockedReadOnlyVertexIndexBase(const unsigned char** vertexbase, int& numverts, PHY_ScalarType& type, int& stride, const unsigned char** indexbase, int& indexstride, int& numfaces, PHY_ScalarType& indicestype, int subpart = 0) const = 0;

	
	
	virtual void unLockVertexBase(int subpart) = 0;

	virtual void unLockReadOnlyVertexBase(int subpart) const = 0;

	
	
	virtual int getNumSubParts() const = 0;

	virtual void preallocateVertices(int numverts) = 0;
	virtual void preallocateIndices(int numindices) = 0;

	virtual bool hasPremadeAabb() const { return false; }
	virtual void setPremadeAabb(const btVector3& aabbMin, const btVector3& aabbMax) const
	{
		(void)aabbMin;
		(void)aabbMax;
	}
	virtual void getPremadeAabb(btVector3 * aabbMin, btVector3 * aabbMax) const
	{
		(void)aabbMin;
		(void)aabbMax;
	}

	const btVector3& getScaling() const
	{
		return m_scaling;
	}
	void setScaling(const btVector3& scaling)
	{
		m_scaling = scaling;
	}

	virtual int calculateSerializeBufferSize() const;

	
	virtual const char* serialize(void* dataBuffer, btSerializer* serializer) const;
};

struct btIntIndexData
{
	int m_value;
};

struct btShortIntIndexData
{
	short m_value;
	char m_pad[2];
};

struct btShortIntIndexTripletData
{
	short m_values[3];
	char m_pad[2];
};

struct btCharIndexTripletData
{
	unsigned char m_values[3];
	char m_pad;
};




struct	btMeshPartData
{
	btVector3FloatData			*m_vertices3f;
	btVector3DoubleData			*m_vertices3d;

	btIntIndexData				*m_indices32;
	btShortIntIndexTripletData	*m_3indices16;
	btCharIndexTripletData		*m_3indices8;

	btShortIntIndexData			*m_indices16;

	int                     m_numTriangles;
	int                     m_numVertices;
};



struct	btStridingMeshInterfaceData
{
	btMeshPartData	*m_meshPartsPtr;
	btVector3FloatData	m_scaling;
	int	m_numMeshParts;
	char m_padding[4];
};



SIMD_FORCE_INLINE int btStridingMeshInterface::calculateSerializeBufferSize() const
{
	return sizeof(btStridingMeshInterfaceData);
}

#endif  
