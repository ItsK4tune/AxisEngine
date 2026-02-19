

#ifndef BT_QUANTIZED_BVH_H
#define BT_QUANTIZED_BVH_H

class btSerializer;


#ifdef DEBUG_CHECK_DEQUANTIZATION
#ifdef __SPU__
#define printf spu_printf
#endif  

#include <stdio.h>
#include <stdlib.h>
#endif  

#include "LinearMath/btVector3.h"
#include "LinearMath/btAlignedAllocator.h"

#ifdef BT_USE_DOUBLE_PRECISION
#define btQuantizedBvhData btQuantizedBvhDoubleData
#define btOptimizedBvhNodeData btOptimizedBvhNodeDoubleData
#define btQuantizedBvhDataName "btQuantizedBvhDoubleData"
#else
#define btQuantizedBvhData btQuantizedBvhFloatData
#define btOptimizedBvhNodeData btOptimizedBvhNodeFloatData
#define btQuantizedBvhDataName "btQuantizedBvhFloatData"
#endif




#define MAX_SUBTREE_SIZE_IN_BYTES 2048





#define MAX_NUM_PARTS_IN_BITS 4



ATTRIBUTE_ALIGNED16(struct)
btQuantizedBvhNode
{
	BT_DECLARE_ALIGNED_ALLOCATOR();

	
	unsigned short int m_quantizedAabbMin[3];
	unsigned short int m_quantizedAabbMax[3];
	
	int m_escapeIndexOrTriangleIndex;

	bool isLeafNode() const
	{
		
		return (m_escapeIndexOrTriangleIndex >= 0);
	}
	int getEscapeIndex() const
	{
		btAssert(!isLeafNode());
		return -m_escapeIndexOrTriangleIndex;
	}
	int getTriangleIndex() const
	{
		btAssert(isLeafNode());
		unsigned int x = 0;
		unsigned int y = (~(x & 0)) << (31 - MAX_NUM_PARTS_IN_BITS);
		
		return (m_escapeIndexOrTriangleIndex & ~(y));
	}
	int getPartId() const
	{
		btAssert(isLeafNode());
		
		return (m_escapeIndexOrTriangleIndex >> (31 - MAX_NUM_PARTS_IN_BITS));
	}
};



ATTRIBUTE_ALIGNED16(struct)
btOptimizedBvhNode
{
	BT_DECLARE_ALIGNED_ALLOCATOR();

	
	btVector3 m_aabbMinOrg;
	btVector3 m_aabbMaxOrg;

	
	int m_escapeIndex;

	
	
	int m_subPart;
	int m_triangleIndex;

	
	char m_padding[20];
};


ATTRIBUTE_ALIGNED16(class)
btBvhSubtreeInfo
{
public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	
	unsigned short int m_quantizedAabbMin[3];
	unsigned short int m_quantizedAabbMax[3];
	
	int m_rootNodeIndex;
	
	int m_subtreeSize;
	int m_padding[3];

	btBvhSubtreeInfo()
	{
		
	}

	void setAabbFromQuantizeNode(const btQuantizedBvhNode& quantizedNode)
	{
		m_quantizedAabbMin[0] = quantizedNode.m_quantizedAabbMin[0];
		m_quantizedAabbMin[1] = quantizedNode.m_quantizedAabbMin[1];
		m_quantizedAabbMin[2] = quantizedNode.m_quantizedAabbMin[2];
		m_quantizedAabbMax[0] = quantizedNode.m_quantizedAabbMax[0];
		m_quantizedAabbMax[1] = quantizedNode.m_quantizedAabbMax[1];
		m_quantizedAabbMax[2] = quantizedNode.m_quantizedAabbMax[2];
	}
};

class btNodeOverlapCallback
{
public:
	virtual ~btNodeOverlapCallback(){};

	virtual void processNode(int subPart, int triangleIndex) = 0;
};

#include "LinearMath/btAlignedAllocator.h"
#include "LinearMath/btAlignedObjectArray.h"


typedef btAlignedObjectArray<btOptimizedBvhNode> NodeArray;
typedef btAlignedObjectArray<btQuantizedBvhNode> QuantizedNodeArray;
typedef btAlignedObjectArray<btBvhSubtreeInfo> BvhSubtreeInfoArray;




ATTRIBUTE_ALIGNED16(class)
btQuantizedBvh
{
public:
	enum btTraversalMode
	{
		TRAVERSAL_STACKLESS = 0,
		TRAVERSAL_STACKLESS_CACHE_FRIENDLY,
		TRAVERSAL_RECURSIVE
	};

protected:
	btVector3 m_bvhAabbMin;
	btVector3 m_bvhAabbMax;
	btVector3 m_bvhQuantization;

	int m_bulletVersion;  

	int m_curNodeIndex;
	
	bool m_useQuantization;

	NodeArray m_leafNodes;
	NodeArray m_contiguousNodes;
	QuantizedNodeArray m_quantizedLeafNodes;
	QuantizedNodeArray m_quantizedContiguousNodes;

	btTraversalMode m_traversalMode;
	BvhSubtreeInfoArray m_SubtreeHeaders;

	
	mutable int m_subtreeHeaderCount;

	
	
	void setInternalNodeAabbMin(int nodeIndex, const btVector3& aabbMin)
	{
		if (m_useQuantization)
		{
			quantize(&m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMin[0], aabbMin, 0);
		}
		else
		{
			m_contiguousNodes[nodeIndex].m_aabbMinOrg = aabbMin;
		}
	}
	void setInternalNodeAabbMax(int nodeIndex, const btVector3& aabbMax)
	{
		if (m_useQuantization)
		{
			quantize(&m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMax[0], aabbMax, 1);
		}
		else
		{
			m_contiguousNodes[nodeIndex].m_aabbMaxOrg = aabbMax;
		}
	}

	btVector3 getAabbMin(int nodeIndex) const
	{
		if (m_useQuantization)
		{
			return unQuantize(&m_quantizedLeafNodes[nodeIndex].m_quantizedAabbMin[0]);
		}
		
		return m_leafNodes[nodeIndex].m_aabbMinOrg;
	}
	btVector3 getAabbMax(int nodeIndex) const
	{
		if (m_useQuantization)
		{
			return unQuantize(&m_quantizedLeafNodes[nodeIndex].m_quantizedAabbMax[0]);
		}
		
		return m_leafNodes[nodeIndex].m_aabbMaxOrg;
	}

	void setInternalNodeEscapeIndex(int nodeIndex, int escapeIndex)
	{
		if (m_useQuantization)
		{
			m_quantizedContiguousNodes[nodeIndex].m_escapeIndexOrTriangleIndex = -escapeIndex;
		}
		else
		{
			m_contiguousNodes[nodeIndex].m_escapeIndex = escapeIndex;
		}
	}

	void mergeInternalNodeAabb(int nodeIndex, const btVector3& newAabbMin, const btVector3& newAabbMax)
	{
		if (m_useQuantization)
		{
			unsigned short int quantizedAabbMin[3];
			unsigned short int quantizedAabbMax[3];
			quantize(quantizedAabbMin, newAabbMin, 0);
			quantize(quantizedAabbMax, newAabbMax, 1);
			for (int i = 0; i < 3; i++)
			{
				if (m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMin[i] > quantizedAabbMin[i])
					m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMin[i] = quantizedAabbMin[i];

				if (m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMax[i] < quantizedAabbMax[i])
					m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMax[i] = quantizedAabbMax[i];
			}
		}
		else
		{
			
			m_contiguousNodes[nodeIndex].m_aabbMinOrg.setMin(newAabbMin);
			m_contiguousNodes[nodeIndex].m_aabbMaxOrg.setMax(newAabbMax);
		}
	}

	void swapLeafNodes(int firstIndex, int secondIndex);

	void assignInternalNodeFromLeafNode(int internalNode, int leafNodeIndex);

protected:
	void buildTree(int startIndex, int endIndex);

	int calcSplittingAxis(int startIndex, int endIndex);

	int sortAndCalcSplittingIndex(int startIndex, int endIndex, int splitAxis);

	void walkStacklessTree(btNodeOverlapCallback * nodeCallback, const btVector3& aabbMin, const btVector3& aabbMax) const;

	void walkStacklessQuantizedTreeAgainstRay(btNodeOverlapCallback * nodeCallback, const btVector3& raySource, const btVector3& rayTarget, const btVector3& aabbMin, const btVector3& aabbMax, int startNodeIndex, int endNodeIndex) const;
	void walkStacklessQuantizedTree(btNodeOverlapCallback * nodeCallback, unsigned short int* quantizedQueryAabbMin, unsigned short int* quantizedQueryAabbMax, int startNodeIndex, int endNodeIndex) const;
	void walkStacklessTreeAgainstRay(btNodeOverlapCallback * nodeCallback, const btVector3& raySource, const btVector3& rayTarget, const btVector3& aabbMin, const btVector3& aabbMax, int startNodeIndex, int endNodeIndex) const;

	
	void walkStacklessQuantizedTreeCacheFriendly(btNodeOverlapCallback * nodeCallback, unsigned short int* quantizedQueryAabbMin, unsigned short int* quantizedQueryAabbMax) const;

	
	void walkRecursiveQuantizedTreeAgainstQueryAabb(const btQuantizedBvhNode* currentNode, btNodeOverlapCallback* nodeCallback, unsigned short int* quantizedQueryAabbMin, unsigned short int* quantizedQueryAabbMax) const;

	
	void walkRecursiveQuantizedTreeAgainstQuantizedTree(const btQuantizedBvhNode* treeNodeA, const btQuantizedBvhNode* treeNodeB, btNodeOverlapCallback* nodeCallback) const;

	void updateSubtreeHeaders(int leftChildNodexIndex, int rightChildNodexIndex);

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btQuantizedBvh();

	virtual ~btQuantizedBvh();

	
	void setQuantizationValues(const btVector3& bvhAabbMin, const btVector3& bvhAabbMax, btScalar quantizationMargin = btScalar(1.0));
	QuantizedNodeArray& getLeafNodeArray() { return m_quantizedLeafNodes; }
	
	void buildInternal();
	

	void reportAabbOverlappingNodex(btNodeOverlapCallback * nodeCallback, const btVector3& aabbMin, const btVector3& aabbMax) const;
	void reportRayOverlappingNodex(btNodeOverlapCallback * nodeCallback, const btVector3& raySource, const btVector3& rayTarget) const;
	void reportBoxCastOverlappingNodex(btNodeOverlapCallback * nodeCallback, const btVector3& raySource, const btVector3& rayTarget, const btVector3& aabbMin, const btVector3& aabbMax) const;

	SIMD_FORCE_INLINE void quantize(unsigned short* out, const btVector3& point, int isMax) const
	{
		btAssert(m_useQuantization);

		btAssert(point.getX() <= m_bvhAabbMax.getX());
		btAssert(point.getY() <= m_bvhAabbMax.getY());
		btAssert(point.getZ() <= m_bvhAabbMax.getZ());

		btAssert(point.getX() >= m_bvhAabbMin.getX());
		btAssert(point.getY() >= m_bvhAabbMin.getY());
		btAssert(point.getZ() >= m_bvhAabbMin.getZ());

		btVector3 v = (point - m_bvhAabbMin) * m_bvhQuantization;
		
		
		
		if (isMax)
		{
			out[0] = (unsigned short)(((unsigned short)(v.getX() + btScalar(1.)) | 1));
			out[1] = (unsigned short)(((unsigned short)(v.getY() + btScalar(1.)) | 1));
			out[2] = (unsigned short)(((unsigned short)(v.getZ() + btScalar(1.)) | 1));
		}
		else
		{
			out[0] = (unsigned short)(((unsigned short)(v.getX()) & 0xfffe));
			out[1] = (unsigned short)(((unsigned short)(v.getY()) & 0xfffe));
			out[2] = (unsigned short)(((unsigned short)(v.getZ()) & 0xfffe));
		}

#ifdef DEBUG_CHECK_DEQUANTIZATION
		btVector3 newPoint = unQuantize(out);
		if (isMax)
		{
			if (newPoint.getX() < point.getX())
			{
				printf("unconservative X, diffX = %f, oldX=%f,newX=%f\n", newPoint.getX() - point.getX(), newPoint.getX(), point.getX());
			}
			if (newPoint.getY() < point.getY())
			{
				printf("unconservative Y, diffY = %f, oldY=%f,newY=%f\n", newPoint.getY() - point.getY(), newPoint.getY(), point.getY());
			}
			if (newPoint.getZ() < point.getZ())
			{
				printf("unconservative Z, diffZ = %f, oldZ=%f,newZ=%f\n", newPoint.getZ() - point.getZ(), newPoint.getZ(), point.getZ());
			}
		}
		else
		{
			if (newPoint.getX() > point.getX())
			{
				printf("unconservative X, diffX = %f, oldX=%f,newX=%f\n", newPoint.getX() - point.getX(), newPoint.getX(), point.getX());
			}
			if (newPoint.getY() > point.getY())
			{
				printf("unconservative Y, diffY = %f, oldY=%f,newY=%f\n", newPoint.getY() - point.getY(), newPoint.getY(), point.getY());
			}
			if (newPoint.getZ() > point.getZ())
			{
				printf("unconservative Z, diffZ = %f, oldZ=%f,newZ=%f\n", newPoint.getZ() - point.getZ(), newPoint.getZ(), point.getZ());
			}
		}
#endif  
	}

	SIMD_FORCE_INLINE void quantizeWithClamp(unsigned short* out, const btVector3& point2, int isMax) const
	{
		btAssert(m_useQuantization);

		btVector3 clampedPoint(point2);
		clampedPoint.setMax(m_bvhAabbMin);
		clampedPoint.setMin(m_bvhAabbMax);

		quantize(out, clampedPoint, isMax);
	}

	SIMD_FORCE_INLINE btVector3 unQuantize(const unsigned short* vecIn) const
	{
		btVector3 vecOut;
		vecOut.setValue(
			(btScalar)(vecIn[0]) / (m_bvhQuantization.getX()),
			(btScalar)(vecIn[1]) / (m_bvhQuantization.getY()),
			(btScalar)(vecIn[2]) / (m_bvhQuantization.getZ()));
		vecOut += m_bvhAabbMin;
		return vecOut;
	}

	
	void setTraversalMode(btTraversalMode traversalMode)
	{
		m_traversalMode = traversalMode;
	}

	SIMD_FORCE_INLINE QuantizedNodeArray& getQuantizedNodeArray()
	{
		return m_quantizedContiguousNodes;
	}

	SIMD_FORCE_INLINE BvhSubtreeInfoArray& getSubtreeInfoArray()
	{
		return m_SubtreeHeaders;
	}

	

	
	unsigned calculateSerializeBufferSize() const;

	
	virtual bool serialize(void* o_alignedDataBuffer, unsigned i_dataBufferSize, bool i_swapEndian) const;

	
	static btQuantizedBvh* deSerializeInPlace(void* i_alignedDataBuffer, unsigned int i_dataBufferSize, bool i_swapEndian);

	static unsigned int getAlignmentSerializationPadding();
	

	virtual int calculateSerializeBufferSizeNew() const;

	
	virtual const char* serialize(void* dataBuffer, btSerializer* serializer) const;

	virtual void deSerializeFloat(struct btQuantizedBvhFloatData & quantizedBvhFloatData);

	virtual void deSerializeDouble(struct btQuantizedBvhDoubleData & quantizedBvhDoubleData);

	

	SIMD_FORCE_INLINE bool isQuantized()
	{
		return m_useQuantization;
	}

private:
	
	
	
	btQuantizedBvh(btQuantizedBvh & other, bool ownsMemory);
};



struct btBvhSubtreeInfoData
{
	int m_rootNodeIndex;
	int m_subtreeSize;
	unsigned short m_quantizedAabbMin[3];
	unsigned short m_quantizedAabbMax[3];
};

struct btOptimizedBvhNodeFloatData
{
	btVector3FloatData m_aabbMinOrg;
	btVector3FloatData m_aabbMaxOrg;
	int m_escapeIndex;
	int m_subPart;
	int m_triangleIndex;
	char m_pad[4];
};

struct btOptimizedBvhNodeDoubleData
{
	btVector3DoubleData m_aabbMinOrg;
	btVector3DoubleData m_aabbMaxOrg;
	int m_escapeIndex;
	int m_subPart;
	int m_triangleIndex;
	char m_pad[4];
};


struct btQuantizedBvhNodeData
{
	unsigned short m_quantizedAabbMin[3];
	unsigned short m_quantizedAabbMax[3];
	int	m_escapeIndexOrTriangleIndex;
};

struct	btQuantizedBvhFloatData
{
	btVector3FloatData			m_bvhAabbMin;
	btVector3FloatData			m_bvhAabbMax;
	btVector3FloatData			m_bvhQuantization;
	int					m_curNodeIndex;
	int					m_useQuantization;
	int					m_numContiguousLeafNodes;
	int					m_numQuantizedContiguousNodes;
	btOptimizedBvhNodeFloatData	*m_contiguousNodesPtr;
	btQuantizedBvhNodeData		*m_quantizedContiguousNodesPtr;
	btBvhSubtreeInfoData	*m_subTreeInfoPtr;
	int					m_traversalMode;
	int					m_numSubtreeHeaders;
	
};

struct	btQuantizedBvhDoubleData
{
	btVector3DoubleData			m_bvhAabbMin;
	btVector3DoubleData			m_bvhAabbMax;
	btVector3DoubleData			m_bvhQuantization;
	int							m_curNodeIndex;
	int							m_useQuantization;
	int							m_numContiguousLeafNodes;
	int							m_numQuantizedContiguousNodes;
	btOptimizedBvhNodeDoubleData	*m_contiguousNodesPtr;
	btQuantizedBvhNodeData			*m_quantizedContiguousNodesPtr;

	int							m_traversalMode;
	int							m_numSubtreeHeaders;
	btBvhSubtreeInfoData		*m_subTreeInfoPtr;
};


SIMD_FORCE_INLINE int btQuantizedBvh::calculateSerializeBufferSizeNew() const
{
	return sizeof(btQuantizedBvhData);
}

#endif  
