



#ifndef BT_OPTIMIZED_BVH_H
#define BT_OPTIMIZED_BVH_H

#include "BulletCollision/BroadphaseCollision/btQuantizedBvh.h"

class btStridingMeshInterface;


ATTRIBUTE_ALIGNED16(class)
btOptimizedBvh : public btQuantizedBvh
{
public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

protected:
public:
	btOptimizedBvh();

	virtual ~btOptimizedBvh();

	void build(btStridingMeshInterface * triangles, bool useQuantizedAabbCompression, const btVector3& bvhAabbMin, const btVector3& bvhAabbMax);

	void refit(btStridingMeshInterface * triangles, const btVector3& aabbMin, const btVector3& aabbMax);

	void refitPartial(btStridingMeshInterface * triangles, const btVector3& aabbMin, const btVector3& aabbMax);

	void updateBvhNodes(btStridingMeshInterface * meshInterface, int firstNode, int endNode, int index);

	
	virtual bool serializeInPlace(void* o_alignedDataBuffer, unsigned i_dataBufferSize, bool i_swapEndian) const
	{
		return btQuantizedBvh::serialize(o_alignedDataBuffer, i_dataBufferSize, i_swapEndian);
	}

	
	static btOptimizedBvh* deSerializeInPlace(void* i_alignedDataBuffer, unsigned int i_dataBufferSize, bool i_swapEndian);
};

#endif  
