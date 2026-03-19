

#include "btQuantizedBvh.h"

#include "LinearMath/btAabbUtil2.h"
#include "LinearMath/btIDebugDraw.h"
#include "LinearMath/btSerializer.h"

#define RAYAABB2

btQuantizedBvh::btQuantizedBvh() : m_bulletVersion(BT_BULLET_VERSION),
								   m_useQuantization(false),
								   
								   m_traversalMode(TRAVERSAL_STACKLESS)
								   
								   ,
								   m_subtreeHeaderCount(0)  
{
	m_bvhAabbMin.setValue(-SIMD_INFINITY, -SIMD_INFINITY, -SIMD_INFINITY);
	m_bvhAabbMax.setValue(SIMD_INFINITY, SIMD_INFINITY, SIMD_INFINITY);
}

void btQuantizedBvh::buildInternal()
{
	
	m_useQuantization = true;
	int numLeafNodes = 0;

	if (m_useQuantization)
	{
		
		numLeafNodes = m_quantizedLeafNodes.size();

		m_quantizedContiguousNodes.resize(2 * numLeafNodes);
	}

	m_curNodeIndex = 0;

	buildTree(0, numLeafNodes);

	
	if (m_useQuantization && !m_SubtreeHeaders.size())
	{
		btBvhSubtreeInfo& subtree = m_SubtreeHeaders.expand();
		subtree.setAabbFromQuantizeNode(m_quantizedContiguousNodes[0]);
		subtree.m_rootNodeIndex = 0;
		subtree.m_subtreeSize = m_quantizedContiguousNodes[0].isLeafNode() ? 1 : m_quantizedContiguousNodes[0].getEscapeIndex();
	}

	
	m_subtreeHeaderCount = m_SubtreeHeaders.size();

	
	m_quantizedLeafNodes.clear();
	m_leafNodes.clear();
}


#ifdef DEBUG_PATCH_COLORS
btVector3 color[4] =
	{
		btVector3(1, 0, 0),
		btVector3(0, 1, 0),
		btVector3(0, 0, 1),
		btVector3(0, 1, 1)};
#endif  

void btQuantizedBvh::setQuantizationValues(const btVector3& bvhAabbMin, const btVector3& bvhAabbMax, btScalar quantizationMargin)
{
	
	btVector3 clampValue(quantizationMargin, quantizationMargin, quantizationMargin);
	m_bvhAabbMin = bvhAabbMin - clampValue;
	m_bvhAabbMax = bvhAabbMax + clampValue;
	btVector3 aabbSize = m_bvhAabbMax - m_bvhAabbMin;
	m_bvhQuantization = btVector3(btScalar(65533.0), btScalar(65533.0), btScalar(65533.0)) / aabbSize;

	m_useQuantization = true;

	{
		unsigned short vecIn[3];
		btVector3 v;
		{
			quantize(vecIn, m_bvhAabbMin, false);
			v = unQuantize(vecIn);
			m_bvhAabbMin.setMin(v - clampValue);
		}
		aabbSize = m_bvhAabbMax - m_bvhAabbMin;
		m_bvhQuantization = btVector3(btScalar(65533.0), btScalar(65533.0), btScalar(65533.0)) / aabbSize;
		{
			quantize(vecIn, m_bvhAabbMax, true);
			v = unQuantize(vecIn);
			m_bvhAabbMax.setMax(v + clampValue);
		}
		aabbSize = m_bvhAabbMax - m_bvhAabbMin;
		m_bvhQuantization = btVector3(btScalar(65533.0), btScalar(65533.0), btScalar(65533.0)) / aabbSize;
	}
}

btQuantizedBvh::~btQuantizedBvh()
{
}

#ifdef DEBUG_TREE_BUILDING
int gStackDepth = 0;
int gMaxStackDepth = 0;
#endif  

void btQuantizedBvh::buildTree(int startIndex, int endIndex)
{
#ifdef DEBUG_TREE_BUILDING
	gStackDepth++;
	if (gStackDepth > gMaxStackDepth)
		gMaxStackDepth = gStackDepth;
#endif  

	int splitAxis, splitIndex, i;
	int numIndices = endIndex - startIndex;
	int curIndex = m_curNodeIndex;

	btAssert(numIndices > 0);

	if (numIndices == 1)
	{
#ifdef DEBUG_TREE_BUILDING
		gStackDepth--;
#endif  

		assignInternalNodeFromLeafNode(m_curNodeIndex, startIndex);

		m_curNodeIndex++;
		return;
	}
	

	splitAxis = calcSplittingAxis(startIndex, endIndex);

	splitIndex = sortAndCalcSplittingIndex(startIndex, endIndex, splitAxis);

	int internalNodeIndex = m_curNodeIndex;

	
	
	setInternalNodeAabbMin(m_curNodeIndex, m_bvhAabbMax);  
	setInternalNodeAabbMax(m_curNodeIndex, m_bvhAabbMin);  

	for (i = startIndex; i < endIndex; i++)
	{
		mergeInternalNodeAabb(m_curNodeIndex, getAabbMin(i), getAabbMax(i));
	}

	m_curNodeIndex++;

	

	int leftChildNodexIndex = m_curNodeIndex;

	
	buildTree(startIndex, splitIndex);

	int rightChildNodexIndex = m_curNodeIndex;
	
	buildTree(splitIndex, endIndex);

#ifdef DEBUG_TREE_BUILDING
	gStackDepth--;
#endif  

	int escapeIndex = m_curNodeIndex - curIndex;

	if (m_useQuantization)
	{
		
		const int sizeQuantizedNode = sizeof(btQuantizedBvhNode);
		const int treeSizeInBytes = escapeIndex * sizeQuantizedNode;
		if (treeSizeInBytes > MAX_SUBTREE_SIZE_IN_BYTES)
		{
			updateSubtreeHeaders(leftChildNodexIndex, rightChildNodexIndex);
		}
	}
	else
	{
	}

	setInternalNodeEscapeIndex(internalNodeIndex, escapeIndex);
}

void btQuantizedBvh::updateSubtreeHeaders(int leftChildNodexIndex, int rightChildNodexIndex)
{
	btAssert(m_useQuantization);

	btQuantizedBvhNode& leftChildNode = m_quantizedContiguousNodes[leftChildNodexIndex];
	int leftSubTreeSize = leftChildNode.isLeafNode() ? 1 : leftChildNode.getEscapeIndex();
	int leftSubTreeSizeInBytes = leftSubTreeSize * static_cast<int>(sizeof(btQuantizedBvhNode));

	btQuantizedBvhNode& rightChildNode = m_quantizedContiguousNodes[rightChildNodexIndex];
	int rightSubTreeSize = rightChildNode.isLeafNode() ? 1 : rightChildNode.getEscapeIndex();
	int rightSubTreeSizeInBytes = rightSubTreeSize * static_cast<int>(sizeof(btQuantizedBvhNode));

	if (leftSubTreeSizeInBytes <= MAX_SUBTREE_SIZE_IN_BYTES)
	{
		btBvhSubtreeInfo& subtree = m_SubtreeHeaders.expand();
		subtree.setAabbFromQuantizeNode(leftChildNode);
		subtree.m_rootNodeIndex = leftChildNodexIndex;
		subtree.m_subtreeSize = leftSubTreeSize;
	}

	if (rightSubTreeSizeInBytes <= MAX_SUBTREE_SIZE_IN_BYTES)
	{
		btBvhSubtreeInfo& subtree = m_SubtreeHeaders.expand();
		subtree.setAabbFromQuantizeNode(rightChildNode);
		subtree.m_rootNodeIndex = rightChildNodexIndex;
		subtree.m_subtreeSize = rightSubTreeSize;
	}

	
	m_subtreeHeaderCount = m_SubtreeHeaders.size();
}

int btQuantizedBvh::sortAndCalcSplittingIndex(int startIndex, int endIndex, int splitAxis)
{
	int i;
	int splitIndex = startIndex;
	int numIndices = endIndex - startIndex;
	btScalar splitValue;

	btVector3 means(btScalar(0.), btScalar(0.), btScalar(0.));
	for (i = startIndex; i < endIndex; i++)
	{
		btVector3 center = btScalar(0.5) * (getAabbMax(i) + getAabbMin(i));
		means += center;
	}
	means *= (btScalar(1.) / (btScalar)numIndices);

	splitValue = means[splitAxis];

	
	for (i = startIndex; i < endIndex; i++)
	{
		btVector3 center = btScalar(0.5) * (getAabbMax(i) + getAabbMin(i));
		if (center[splitAxis] > splitValue)
		{
			
			swapLeafNodes(i, splitIndex);
			splitIndex++;
		}
	}

	
	
	
	

	
	

	
	int rangeBalancedIndices = numIndices / 3;
	bool unbalanced = ((splitIndex <= (startIndex + rangeBalancedIndices)) || (splitIndex >= (endIndex - 1 - rangeBalancedIndices)));

	if (unbalanced)
	{
		splitIndex = startIndex + (numIndices >> 1);
	}

	bool unbal = (splitIndex == startIndex) || (splitIndex == (endIndex));
	(void)unbal;
	btAssert(!unbal);

	return splitIndex;
}

int btQuantizedBvh::calcSplittingAxis(int startIndex, int endIndex)
{
	int i;

	btVector3 means(btScalar(0.), btScalar(0.), btScalar(0.));
	btVector3 variance(btScalar(0.), btScalar(0.), btScalar(0.));
	int numIndices = endIndex - startIndex;

	for (i = startIndex; i < endIndex; i++)
	{
		btVector3 center = btScalar(0.5) * (getAabbMax(i) + getAabbMin(i));
		means += center;
	}
	means *= (btScalar(1.) / (btScalar)numIndices);

	for (i = startIndex; i < endIndex; i++)
	{
		btVector3 center = btScalar(0.5) * (getAabbMax(i) + getAabbMin(i));
		btVector3 diff2 = center - means;
		diff2 = diff2 * diff2;
		variance += diff2;
	}
	variance *= (btScalar(1.) / ((btScalar)numIndices - 1));

	return variance.maxAxis();
}

void btQuantizedBvh::reportAabbOverlappingNodex(btNodeOverlapCallback* nodeCallback, const btVector3& aabbMin, const btVector3& aabbMax) const
{
	

	if (m_useQuantization)
	{
		
		unsigned short int quantizedQueryAabbMin[3];
		unsigned short int quantizedQueryAabbMax[3];
		quantizeWithClamp(quantizedQueryAabbMin, aabbMin, 0);
		quantizeWithClamp(quantizedQueryAabbMax, aabbMax, 1);

		switch (m_traversalMode)
		{
			case TRAVERSAL_STACKLESS:
				walkStacklessQuantizedTree(nodeCallback, quantizedQueryAabbMin, quantizedQueryAabbMax, 0, m_curNodeIndex);
				break;
			case TRAVERSAL_STACKLESS_CACHE_FRIENDLY:
				walkStacklessQuantizedTreeCacheFriendly(nodeCallback, quantizedQueryAabbMin, quantizedQueryAabbMax);
				break;
			case TRAVERSAL_RECURSIVE:
			{
				const btQuantizedBvhNode* rootNode = &m_quantizedContiguousNodes[0];
				walkRecursiveQuantizedTreeAgainstQueryAabb(rootNode, nodeCallback, quantizedQueryAabbMin, quantizedQueryAabbMax);
			}
			break;
			default:
				
				btAssert(0);
		}
	}
	else
	{
		walkStacklessTree(nodeCallback, aabbMin, aabbMax);
	}
}

void btQuantizedBvh::walkStacklessTree(btNodeOverlapCallback* nodeCallback, const btVector3& aabbMin, const btVector3& aabbMax) const
{
	btAssert(!m_useQuantization);

	const btOptimizedBvhNode* rootNode = &m_contiguousNodes[0];
	int escapeIndex, curIndex = 0;
	int walkIterations = 0;
	bool isLeafNode;
	
	unsigned aabbOverlap;

	while (curIndex < m_curNodeIndex)
	{
		
		btAssert(walkIterations < m_curNodeIndex);

		walkIterations++;
		aabbOverlap = TestAabbAgainstAabb2(aabbMin, aabbMax, rootNode->m_aabbMinOrg, rootNode->m_aabbMaxOrg);
		isLeafNode = rootNode->m_escapeIndex == -1;

		
		if (isLeafNode && (aabbOverlap != 0))
		{
			nodeCallback->processNode(rootNode->m_subPart, rootNode->m_triangleIndex);
		}

		
		if ((aabbOverlap != 0) || isLeafNode)
		{
			rootNode++;
			curIndex++;
		}
		else
		{
			escapeIndex = rootNode->m_escapeIndex;
			rootNode += escapeIndex;
			curIndex += escapeIndex;
		}
	}
}



void btQuantizedBvh::walkRecursiveQuantizedTreeAgainstQueryAabb(const btQuantizedBvhNode* currentNode, btNodeOverlapCallback* nodeCallback, unsigned short int* quantizedQueryAabbMin, unsigned short int* quantizedQueryAabbMax) const
{
	btAssert(m_useQuantization);

	bool isLeafNode;
	
	unsigned aabbOverlap;

	
	aabbOverlap = testQuantizedAabbAgainstQuantizedAabb(quantizedQueryAabbMin, quantizedQueryAabbMax, currentNode->m_quantizedAabbMin, currentNode->m_quantizedAabbMax);
	isLeafNode = currentNode->isLeafNode();

	
	if (aabbOverlap != 0)
	{
		if (isLeafNode)
		{
			nodeCallback->processNode(currentNode->getPartId(), currentNode->getTriangleIndex());
		}
		else
		{
			
			const btQuantizedBvhNode* leftChildNode = currentNode + 1;
			walkRecursiveQuantizedTreeAgainstQueryAabb(leftChildNode, nodeCallback, quantizedQueryAabbMin, quantizedQueryAabbMax);

			const btQuantizedBvhNode* rightChildNode = leftChildNode->isLeafNode() ? leftChildNode + 1 : leftChildNode + leftChildNode->getEscapeIndex();
			walkRecursiveQuantizedTreeAgainstQueryAabb(rightChildNode, nodeCallback, quantizedQueryAabbMin, quantizedQueryAabbMax);
		}
	}
}

void btQuantizedBvh::walkStacklessTreeAgainstRay(btNodeOverlapCallback* nodeCallback, const btVector3& raySource, const btVector3& rayTarget, const btVector3& aabbMin, const btVector3& aabbMax, int startNodeIndex, int endNodeIndex) const
{
	btAssert(!m_useQuantization);

	const btOptimizedBvhNode* rootNode = &m_contiguousNodes[0];
	int escapeIndex, curIndex = 0;
	int walkIterations = 0;
	bool isLeafNode;
	
	unsigned aabbOverlap = 0;
	unsigned rayBoxOverlap = 0;
	btScalar lambda_max = 1.0;

	
	btVector3 rayAabbMin = raySource;
	btVector3 rayAabbMax = raySource;
	rayAabbMin.setMin(rayTarget);
	rayAabbMax.setMax(rayTarget);

	
	rayAabbMin += aabbMin;
	rayAabbMax += aabbMax;

#ifdef RAYAABB2
	btVector3 rayDir = (rayTarget - raySource);
	rayDir.safeNormalize();
	lambda_max = rayDir.dot(rayTarget - raySource);
	
	btVector3 rayDirectionInverse;
	rayDirectionInverse[0] = rayDir[0] == btScalar(0.0) ? btScalar(BT_LARGE_FLOAT) : btScalar(1.0) / rayDir[0];
	rayDirectionInverse[1] = rayDir[1] == btScalar(0.0) ? btScalar(BT_LARGE_FLOAT) : btScalar(1.0) / rayDir[1];
	rayDirectionInverse[2] = rayDir[2] == btScalar(0.0) ? btScalar(BT_LARGE_FLOAT) : btScalar(1.0) / rayDir[2];
	unsigned int sign[3] = {rayDirectionInverse[0] < 0.0, rayDirectionInverse[1] < 0.0, rayDirectionInverse[2] < 0.0};
#endif

	btVector3 bounds[2];

	while (curIndex < m_curNodeIndex)
	{
		btScalar param = 1.0;
		
		btAssert(walkIterations < m_curNodeIndex);

		walkIterations++;

		bounds[0] = rootNode->m_aabbMinOrg;
		bounds[1] = rootNode->m_aabbMaxOrg;
		
		bounds[0] -= aabbMax;
		bounds[1] -= aabbMin;

		aabbOverlap = TestAabbAgainstAabb2(rayAabbMin, rayAabbMax, rootNode->m_aabbMinOrg, rootNode->m_aabbMaxOrg);
		

#ifdef RAYAABB2
		
		
		
		rayBoxOverlap = aabbOverlap ? btRayAabb2(raySource, rayDirectionInverse, sign, bounds, param, 0.0f, lambda_max) : false;

#else
		btVector3 normal;
		rayBoxOverlap = btRayAabb(raySource, rayTarget, bounds[0], bounds[1], param, normal);
#endif

		isLeafNode = rootNode->m_escapeIndex == -1;

		
		if (isLeafNode && (rayBoxOverlap != 0))
		{
			nodeCallback->processNode(rootNode->m_subPart, rootNode->m_triangleIndex);
		}

		
		if ((rayBoxOverlap != 0) || isLeafNode)
		{
			rootNode++;
			curIndex++;
		}
		else
		{
			escapeIndex = rootNode->m_escapeIndex;
			rootNode += escapeIndex;
			curIndex += escapeIndex;
		}
	}
}

void btQuantizedBvh::walkStacklessQuantizedTreeAgainstRay(btNodeOverlapCallback* nodeCallback, const btVector3& raySource, const btVector3& rayTarget, const btVector3& aabbMin, const btVector3& aabbMax, int startNodeIndex, int endNodeIndex) const
{
	btAssert(m_useQuantization);

	int curIndex = startNodeIndex;
	int walkIterations = 0;
	int subTreeSize = endNodeIndex - startNodeIndex;
	(void)subTreeSize;

	const btQuantizedBvhNode* rootNode = &m_quantizedContiguousNodes[startNodeIndex];
	int escapeIndex;

	bool isLeafNode;
	
	unsigned boxBoxOverlap = 0;
	unsigned rayBoxOverlap = 0;

	btScalar lambda_max = 1.0;

#ifdef RAYAABB2
	btVector3 rayDirection = (rayTarget - raySource);
	rayDirection.safeNormalize();
	lambda_max = rayDirection.dot(rayTarget - raySource);
	
	rayDirection[0] = rayDirection[0] == btScalar(0.0) ? btScalar(BT_LARGE_FLOAT) : btScalar(1.0) / rayDirection[0];
	rayDirection[1] = rayDirection[1] == btScalar(0.0) ? btScalar(BT_LARGE_FLOAT) : btScalar(1.0) / rayDirection[1];
	rayDirection[2] = rayDirection[2] == btScalar(0.0) ? btScalar(BT_LARGE_FLOAT) : btScalar(1.0) / rayDirection[2];
	unsigned int sign[3] = {rayDirection[0] < 0.0, rayDirection[1] < 0.0, rayDirection[2] < 0.0};
#endif

	
	btVector3 rayAabbMin = raySource;
	btVector3 rayAabbMax = raySource;
	rayAabbMin.setMin(rayTarget);
	rayAabbMax.setMax(rayTarget);

	
	rayAabbMin += aabbMin;
	rayAabbMax += aabbMax;

	unsigned short int quantizedQueryAabbMin[3];
	unsigned short int quantizedQueryAabbMax[3];
	quantizeWithClamp(quantizedQueryAabbMin, rayAabbMin, 0);
	quantizeWithClamp(quantizedQueryAabbMax, rayAabbMax, 1);

	while (curIndex < endNodeIndex)
	{

#ifdef VISUALLY_ANALYZE_BVH
		
		static int drawPatch = 0;
		
		extern btIDebugDraw* debugDrawerPtr;
		if (curIndex == drawPatch)
		{
			btVector3 aabbMin, aabbMax;
			aabbMin = unQuantize(rootNode->m_quantizedAabbMin);
			aabbMax = unQuantize(rootNode->m_quantizedAabbMax);
			btVector3 color(1, 0, 0);
			debugDrawerPtr->drawAabb(aabbMin, aabbMax, color);
		}
#endif  

		
		btAssert(walkIterations < subTreeSize);

		walkIterations++;
		
		
		btScalar param = 1.0;
		rayBoxOverlap = 0;
		boxBoxOverlap = testQuantizedAabbAgainstQuantizedAabb(quantizedQueryAabbMin, quantizedQueryAabbMax, rootNode->m_quantizedAabbMin, rootNode->m_quantizedAabbMax);
		isLeafNode = rootNode->isLeafNode();
		if (boxBoxOverlap)
		{
			btVector3 bounds[2];
			bounds[0] = unQuantize(rootNode->m_quantizedAabbMin);
			bounds[1] = unQuantize(rootNode->m_quantizedAabbMax);
			
			bounds[0] -= aabbMax;
			bounds[1] -= aabbMin;
			btVector3 normal;
#if 0
			bool ra2 = btRayAabb2 (raySource, rayDirection, sign, bounds, param, 0.0, lambda_max);
			bool ra = btRayAabb (raySource, rayTarget, bounds[0], bounds[1], param, normal);
			if (ra2 != ra)
			{
				printf("functions don't match\n");
			}
#endif
#ifdef RAYAABB2
			
			
			

			
			rayBoxOverlap = btRayAabb2(raySource, rayDirection, sign, bounds, param, 0.0f, lambda_max);

#else
			rayBoxOverlap = true;  
#endif
		}

		if (isLeafNode && rayBoxOverlap)
		{
			nodeCallback->processNode(rootNode->getPartId(), rootNode->getTriangleIndex());
		}

		
		if ((rayBoxOverlap != 0) || isLeafNode)
		{
			rootNode++;
			curIndex++;
		}
		else
		{
			escapeIndex = rootNode->getEscapeIndex();
			rootNode += escapeIndex;
			curIndex += escapeIndex;
		}
	}
}

void btQuantizedBvh::walkStacklessQuantizedTree(btNodeOverlapCallback* nodeCallback, unsigned short int* quantizedQueryAabbMin, unsigned short int* quantizedQueryAabbMax, int startNodeIndex, int endNodeIndex) const
{
	btAssert(m_useQuantization);

	int curIndex = startNodeIndex;
	int walkIterations = 0;
	int subTreeSize = endNodeIndex - startNodeIndex;
	(void)subTreeSize;

	const btQuantizedBvhNode* rootNode = &m_quantizedContiguousNodes[startNodeIndex];
	int escapeIndex;

	bool isLeafNode;
	
	unsigned aabbOverlap;

	while (curIndex < endNodeIndex)
	{

#ifdef VISUALLY_ANALYZE_BVH
		
		static int drawPatch = 0;
		
		extern btIDebugDraw* debugDrawerPtr;
		if (curIndex == drawPatch)
		{
			btVector3 aabbMin, aabbMax;
			aabbMin = unQuantize(rootNode->m_quantizedAabbMin);
			aabbMax = unQuantize(rootNode->m_quantizedAabbMax);
			btVector3 color(1, 0, 0);
			debugDrawerPtr->drawAabb(aabbMin, aabbMax, color);
		}
#endif  

		
		btAssert(walkIterations < subTreeSize);

		walkIterations++;
		
		aabbOverlap = testQuantizedAabbAgainstQuantizedAabb(quantizedQueryAabbMin, quantizedQueryAabbMax, rootNode->m_quantizedAabbMin, rootNode->m_quantizedAabbMax);
		isLeafNode = rootNode->isLeafNode();

		if (isLeafNode && aabbOverlap)
		{
			nodeCallback->processNode(rootNode->getPartId(), rootNode->getTriangleIndex());
		}

		
		if ((aabbOverlap != 0) || isLeafNode)
		{
			rootNode++;
			curIndex++;
		}
		else
		{
			escapeIndex = rootNode->getEscapeIndex();
			rootNode += escapeIndex;
			curIndex += escapeIndex;
		}
	}
}


void btQuantizedBvh::walkStacklessQuantizedTreeCacheFriendly(btNodeOverlapCallback* nodeCallback, unsigned short int* quantizedQueryAabbMin, unsigned short int* quantizedQueryAabbMax) const
{
	btAssert(m_useQuantization);

	int i;

	for (i = 0; i < this->m_SubtreeHeaders.size(); i++)
	{
		const btBvhSubtreeInfo& subtree = m_SubtreeHeaders[i];

		
		unsigned overlap = testQuantizedAabbAgainstQuantizedAabb(quantizedQueryAabbMin, quantizedQueryAabbMax, subtree.m_quantizedAabbMin, subtree.m_quantizedAabbMax);
		if (overlap != 0)
		{
			walkStacklessQuantizedTree(nodeCallback, quantizedQueryAabbMin, quantizedQueryAabbMax,
									   subtree.m_rootNodeIndex,
									   subtree.m_rootNodeIndex + subtree.m_subtreeSize);
		}
	}
}

void btQuantizedBvh::reportRayOverlappingNodex(btNodeOverlapCallback* nodeCallback, const btVector3& raySource, const btVector3& rayTarget) const
{
	reportBoxCastOverlappingNodex(nodeCallback, raySource, rayTarget, btVector3(0, 0, 0), btVector3(0, 0, 0));
}

void btQuantizedBvh::reportBoxCastOverlappingNodex(btNodeOverlapCallback* nodeCallback, const btVector3& raySource, const btVector3& rayTarget, const btVector3& aabbMin, const btVector3& aabbMax) const
{
	

	if (m_useQuantization)
	{
		walkStacklessQuantizedTreeAgainstRay(nodeCallback, raySource, rayTarget, aabbMin, aabbMax, 0, m_curNodeIndex);
	}
	else
	{
		walkStacklessTreeAgainstRay(nodeCallback, raySource, rayTarget, aabbMin, aabbMax, 0, m_curNodeIndex);
	}
	
}

void btQuantizedBvh::swapLeafNodes(int i, int splitIndex)
{
	if (m_useQuantization)
	{
		btQuantizedBvhNode tmp = m_quantizedLeafNodes[i];
		m_quantizedLeafNodes[i] = m_quantizedLeafNodes[splitIndex];
		m_quantizedLeafNodes[splitIndex] = tmp;
	}
	else
	{
		btOptimizedBvhNode tmp = m_leafNodes[i];
		m_leafNodes[i] = m_leafNodes[splitIndex];
		m_leafNodes[splitIndex] = tmp;
	}
}

void btQuantizedBvh::assignInternalNodeFromLeafNode(int internalNode, int leafNodeIndex)
{
	if (m_useQuantization)
	{
		m_quantizedContiguousNodes[internalNode] = m_quantizedLeafNodes[leafNodeIndex];
	}
	else
	{
		m_contiguousNodes[internalNode] = m_leafNodes[leafNodeIndex];
	}
}


#include <new>

#if 0

static const unsigned BVH_ALIGNMENT = 16;
static const unsigned BVH_ALIGNMENT_MASK = BVH_ALIGNMENT-1;

static const unsigned BVH_ALIGNMENT_BLOCKS = 2;
#endif

unsigned int btQuantizedBvh::getAlignmentSerializationPadding()
{
	
	return 0;  
}

unsigned btQuantizedBvh::calculateSerializeBufferSize() const
{
	unsigned baseSize = sizeof(btQuantizedBvh) + getAlignmentSerializationPadding();
	baseSize += sizeof(btBvhSubtreeInfo) * m_subtreeHeaderCount;
	if (m_useQuantization)
	{
		return baseSize + m_curNodeIndex * sizeof(btQuantizedBvhNode);
	}
	return baseSize + m_curNodeIndex * sizeof(btOptimizedBvhNode);
}

bool btQuantizedBvh::serialize(void* o_alignedDataBuffer, unsigned , bool i_swapEndian) const
{
	btAssert(m_subtreeHeaderCount == m_SubtreeHeaders.size());
	m_subtreeHeaderCount = m_SubtreeHeaders.size();

	

	btQuantizedBvh* targetBvh = (btQuantizedBvh*)o_alignedDataBuffer;

	
	
	new (targetBvh) btQuantizedBvh;

	if (i_swapEndian)
	{
		targetBvh->m_curNodeIndex = static_cast<int>(btSwapEndian(m_curNodeIndex));

		btSwapVector3Endian(m_bvhAabbMin, targetBvh->m_bvhAabbMin);
		btSwapVector3Endian(m_bvhAabbMax, targetBvh->m_bvhAabbMax);
		btSwapVector3Endian(m_bvhQuantization, targetBvh->m_bvhQuantization);

		targetBvh->m_traversalMode = (btTraversalMode)btSwapEndian(m_traversalMode);
		targetBvh->m_subtreeHeaderCount = static_cast<int>(btSwapEndian(m_subtreeHeaderCount));
	}
	else
	{
		targetBvh->m_curNodeIndex = m_curNodeIndex;
		targetBvh->m_bvhAabbMin = m_bvhAabbMin;
		targetBvh->m_bvhAabbMax = m_bvhAabbMax;
		targetBvh->m_bvhQuantization = m_bvhQuantization;
		targetBvh->m_traversalMode = m_traversalMode;
		targetBvh->m_subtreeHeaderCount = m_subtreeHeaderCount;
	}

	targetBvh->m_useQuantization = m_useQuantization;

	unsigned char* nodeData = (unsigned char*)targetBvh;
	nodeData += sizeof(btQuantizedBvh);

	unsigned sizeToAdd = 0;  
	nodeData += sizeToAdd;

	int nodeCount = m_curNodeIndex;

	if (m_useQuantization)
	{
		targetBvh->m_quantizedContiguousNodes.initializeFromBuffer(nodeData, nodeCount, nodeCount);

		if (i_swapEndian)
		{
			for (int nodeIndex = 0; nodeIndex < nodeCount; nodeIndex++)
			{
				targetBvh->m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMin[0] = btSwapEndian(m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMin[0]);
				targetBvh->m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMin[1] = btSwapEndian(m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMin[1]);
				targetBvh->m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMin[2] = btSwapEndian(m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMin[2]);

				targetBvh->m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMax[0] = btSwapEndian(m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMax[0]);
				targetBvh->m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMax[1] = btSwapEndian(m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMax[1]);
				targetBvh->m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMax[2] = btSwapEndian(m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMax[2]);

				targetBvh->m_quantizedContiguousNodes[nodeIndex].m_escapeIndexOrTriangleIndex = static_cast<int>(btSwapEndian(m_quantizedContiguousNodes[nodeIndex].m_escapeIndexOrTriangleIndex));
			}
		}
		else
		{
			for (int nodeIndex = 0; nodeIndex < nodeCount; nodeIndex++)
			{
				targetBvh->m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMin[0] = m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMin[0];
				targetBvh->m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMin[1] = m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMin[1];
				targetBvh->m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMin[2] = m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMin[2];

				targetBvh->m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMax[0] = m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMax[0];
				targetBvh->m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMax[1] = m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMax[1];
				targetBvh->m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMax[2] = m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMax[2];

				targetBvh->m_quantizedContiguousNodes[nodeIndex].m_escapeIndexOrTriangleIndex = m_quantizedContiguousNodes[nodeIndex].m_escapeIndexOrTriangleIndex;
			}
		}
		nodeData += sizeof(btQuantizedBvhNode) * nodeCount;

		
		
		
		targetBvh->m_quantizedContiguousNodes.initializeFromBuffer(NULL, 0, 0);
	}
	else
	{
		targetBvh->m_contiguousNodes.initializeFromBuffer(nodeData, nodeCount, nodeCount);

		if (i_swapEndian)
		{
			for (int nodeIndex = 0; nodeIndex < nodeCount; nodeIndex++)
			{
				btSwapVector3Endian(m_contiguousNodes[nodeIndex].m_aabbMinOrg, targetBvh->m_contiguousNodes[nodeIndex].m_aabbMinOrg);
				btSwapVector3Endian(m_contiguousNodes[nodeIndex].m_aabbMaxOrg, targetBvh->m_contiguousNodes[nodeIndex].m_aabbMaxOrg);

				targetBvh->m_contiguousNodes[nodeIndex].m_escapeIndex = static_cast<int>(btSwapEndian(m_contiguousNodes[nodeIndex].m_escapeIndex));
				targetBvh->m_contiguousNodes[nodeIndex].m_subPart = static_cast<int>(btSwapEndian(m_contiguousNodes[nodeIndex].m_subPart));
				targetBvh->m_contiguousNodes[nodeIndex].m_triangleIndex = static_cast<int>(btSwapEndian(m_contiguousNodes[nodeIndex].m_triangleIndex));
			}
		}
		else
		{
			for (int nodeIndex = 0; nodeIndex < nodeCount; nodeIndex++)
			{
				targetBvh->m_contiguousNodes[nodeIndex].m_aabbMinOrg = m_contiguousNodes[nodeIndex].m_aabbMinOrg;
				targetBvh->m_contiguousNodes[nodeIndex].m_aabbMaxOrg = m_contiguousNodes[nodeIndex].m_aabbMaxOrg;

				targetBvh->m_contiguousNodes[nodeIndex].m_escapeIndex = m_contiguousNodes[nodeIndex].m_escapeIndex;
				targetBvh->m_contiguousNodes[nodeIndex].m_subPart = m_contiguousNodes[nodeIndex].m_subPart;
				targetBvh->m_contiguousNodes[nodeIndex].m_triangleIndex = m_contiguousNodes[nodeIndex].m_triangleIndex;
			}
		}
		nodeData += sizeof(btOptimizedBvhNode) * nodeCount;

		
		
		
		targetBvh->m_contiguousNodes.initializeFromBuffer(NULL, 0, 0);
	}

	sizeToAdd = 0;  
	nodeData += sizeToAdd;

	
	targetBvh->m_SubtreeHeaders.initializeFromBuffer(nodeData, m_subtreeHeaderCount, m_subtreeHeaderCount);
	if (i_swapEndian)
	{
		for (int i = 0; i < m_subtreeHeaderCount; i++)
		{
			targetBvh->m_SubtreeHeaders[i].m_quantizedAabbMin[0] = btSwapEndian(m_SubtreeHeaders[i].m_quantizedAabbMin[0]);
			targetBvh->m_SubtreeHeaders[i].m_quantizedAabbMin[1] = btSwapEndian(m_SubtreeHeaders[i].m_quantizedAabbMin[1]);
			targetBvh->m_SubtreeHeaders[i].m_quantizedAabbMin[2] = btSwapEndian(m_SubtreeHeaders[i].m_quantizedAabbMin[2]);

			targetBvh->m_SubtreeHeaders[i].m_quantizedAabbMax[0] = btSwapEndian(m_SubtreeHeaders[i].m_quantizedAabbMax[0]);
			targetBvh->m_SubtreeHeaders[i].m_quantizedAabbMax[1] = btSwapEndian(m_SubtreeHeaders[i].m_quantizedAabbMax[1]);
			targetBvh->m_SubtreeHeaders[i].m_quantizedAabbMax[2] = btSwapEndian(m_SubtreeHeaders[i].m_quantizedAabbMax[2]);

			targetBvh->m_SubtreeHeaders[i].m_rootNodeIndex = static_cast<int>(btSwapEndian(m_SubtreeHeaders[i].m_rootNodeIndex));
			targetBvh->m_SubtreeHeaders[i].m_subtreeSize = static_cast<int>(btSwapEndian(m_SubtreeHeaders[i].m_subtreeSize));
		}
	}
	else
	{
		for (int i = 0; i < m_subtreeHeaderCount; i++)
		{
			targetBvh->m_SubtreeHeaders[i].m_quantizedAabbMin[0] = (m_SubtreeHeaders[i].m_quantizedAabbMin[0]);
			targetBvh->m_SubtreeHeaders[i].m_quantizedAabbMin[1] = (m_SubtreeHeaders[i].m_quantizedAabbMin[1]);
			targetBvh->m_SubtreeHeaders[i].m_quantizedAabbMin[2] = (m_SubtreeHeaders[i].m_quantizedAabbMin[2]);

			targetBvh->m_SubtreeHeaders[i].m_quantizedAabbMax[0] = (m_SubtreeHeaders[i].m_quantizedAabbMax[0]);
			targetBvh->m_SubtreeHeaders[i].m_quantizedAabbMax[1] = (m_SubtreeHeaders[i].m_quantizedAabbMax[1]);
			targetBvh->m_SubtreeHeaders[i].m_quantizedAabbMax[2] = (m_SubtreeHeaders[i].m_quantizedAabbMax[2]);

			targetBvh->m_SubtreeHeaders[i].m_rootNodeIndex = (m_SubtreeHeaders[i].m_rootNodeIndex);
			targetBvh->m_SubtreeHeaders[i].m_subtreeSize = (m_SubtreeHeaders[i].m_subtreeSize);

			
			targetBvh->m_SubtreeHeaders[i].m_padding[0] = 0;
			targetBvh->m_SubtreeHeaders[i].m_padding[1] = 0;
			targetBvh->m_SubtreeHeaders[i].m_padding[2] = 0;
		}
	}
	nodeData += sizeof(btBvhSubtreeInfo) * m_subtreeHeaderCount;

	
	
	
	targetBvh->m_SubtreeHeaders.initializeFromBuffer(NULL, 0, 0);

	
	*((void**)o_alignedDataBuffer) = NULL;

	return true;
}

btQuantizedBvh* btQuantizedBvh::deSerializeInPlace(void* i_alignedDataBuffer, unsigned int i_dataBufferSize, bool i_swapEndian)
{
	if (i_alignedDataBuffer == NULL)  
	{
		return NULL;
	}
	btQuantizedBvh* bvh = (btQuantizedBvh*)i_alignedDataBuffer;

	if (i_swapEndian)
	{
		bvh->m_curNodeIndex = static_cast<int>(btSwapEndian(bvh->m_curNodeIndex));

		btUnSwapVector3Endian(bvh->m_bvhAabbMin);
		btUnSwapVector3Endian(bvh->m_bvhAabbMax);
		btUnSwapVector3Endian(bvh->m_bvhQuantization);

		bvh->m_traversalMode = (btTraversalMode)btSwapEndian(bvh->m_traversalMode);
		bvh->m_subtreeHeaderCount = static_cast<int>(btSwapEndian(bvh->m_subtreeHeaderCount));
	}

	unsigned int calculatedBufSize = bvh->calculateSerializeBufferSize();
	btAssert(calculatedBufSize <= i_dataBufferSize);

	if (calculatedBufSize > i_dataBufferSize)
	{
		return NULL;
	}

	unsigned char* nodeData = (unsigned char*)bvh;
	nodeData += sizeof(btQuantizedBvh);

	unsigned sizeToAdd = 0;  
	nodeData += sizeToAdd;

	int nodeCount = bvh->m_curNodeIndex;

	
	
	new (bvh) btQuantizedBvh(*bvh, false);

	if (bvh->m_useQuantization)
	{
		bvh->m_quantizedContiguousNodes.initializeFromBuffer(nodeData, nodeCount, nodeCount);

		if (i_swapEndian)
		{
			for (int nodeIndex = 0; nodeIndex < nodeCount; nodeIndex++)
			{
				bvh->m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMin[0] = btSwapEndian(bvh->m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMin[0]);
				bvh->m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMin[1] = btSwapEndian(bvh->m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMin[1]);
				bvh->m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMin[2] = btSwapEndian(bvh->m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMin[2]);

				bvh->m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMax[0] = btSwapEndian(bvh->m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMax[0]);
				bvh->m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMax[1] = btSwapEndian(bvh->m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMax[1]);
				bvh->m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMax[2] = btSwapEndian(bvh->m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMax[2]);

				bvh->m_quantizedContiguousNodes[nodeIndex].m_escapeIndexOrTriangleIndex = static_cast<int>(btSwapEndian(bvh->m_quantizedContiguousNodes[nodeIndex].m_escapeIndexOrTriangleIndex));
			}
		}
		nodeData += sizeof(btQuantizedBvhNode) * nodeCount;
	}
	else
	{
		bvh->m_contiguousNodes.initializeFromBuffer(nodeData, nodeCount, nodeCount);

		if (i_swapEndian)
		{
			for (int nodeIndex = 0; nodeIndex < nodeCount; nodeIndex++)
			{
				btUnSwapVector3Endian(bvh->m_contiguousNodes[nodeIndex].m_aabbMinOrg);
				btUnSwapVector3Endian(bvh->m_contiguousNodes[nodeIndex].m_aabbMaxOrg);

				bvh->m_contiguousNodes[nodeIndex].m_escapeIndex = static_cast<int>(btSwapEndian(bvh->m_contiguousNodes[nodeIndex].m_escapeIndex));
				bvh->m_contiguousNodes[nodeIndex].m_subPart = static_cast<int>(btSwapEndian(bvh->m_contiguousNodes[nodeIndex].m_subPart));
				bvh->m_contiguousNodes[nodeIndex].m_triangleIndex = static_cast<int>(btSwapEndian(bvh->m_contiguousNodes[nodeIndex].m_triangleIndex));
			}
		}
		nodeData += sizeof(btOptimizedBvhNode) * nodeCount;
	}

	sizeToAdd = 0;  
	nodeData += sizeToAdd;

	
	bvh->m_SubtreeHeaders.initializeFromBuffer(nodeData, bvh->m_subtreeHeaderCount, bvh->m_subtreeHeaderCount);
	if (i_swapEndian)
	{
		for (int i = 0; i < bvh->m_subtreeHeaderCount; i++)
		{
			bvh->m_SubtreeHeaders[i].m_quantizedAabbMin[0] = btSwapEndian(bvh->m_SubtreeHeaders[i].m_quantizedAabbMin[0]);
			bvh->m_SubtreeHeaders[i].m_quantizedAabbMin[1] = btSwapEndian(bvh->m_SubtreeHeaders[i].m_quantizedAabbMin[1]);
			bvh->m_SubtreeHeaders[i].m_quantizedAabbMin[2] = btSwapEndian(bvh->m_SubtreeHeaders[i].m_quantizedAabbMin[2]);

			bvh->m_SubtreeHeaders[i].m_quantizedAabbMax[0] = btSwapEndian(bvh->m_SubtreeHeaders[i].m_quantizedAabbMax[0]);
			bvh->m_SubtreeHeaders[i].m_quantizedAabbMax[1] = btSwapEndian(bvh->m_SubtreeHeaders[i].m_quantizedAabbMax[1]);
			bvh->m_SubtreeHeaders[i].m_quantizedAabbMax[2] = btSwapEndian(bvh->m_SubtreeHeaders[i].m_quantizedAabbMax[2]);

			bvh->m_SubtreeHeaders[i].m_rootNodeIndex = static_cast<int>(btSwapEndian(bvh->m_SubtreeHeaders[i].m_rootNodeIndex));
			bvh->m_SubtreeHeaders[i].m_subtreeSize = static_cast<int>(btSwapEndian(bvh->m_SubtreeHeaders[i].m_subtreeSize));
		}
	}

	return bvh;
}


btQuantizedBvh::btQuantizedBvh(btQuantizedBvh& self, bool ) : m_bvhAabbMin(self.m_bvhAabbMin),
																			  m_bvhAabbMax(self.m_bvhAabbMax),
																			  m_bvhQuantization(self.m_bvhQuantization),
																			  m_bulletVersion(BT_BULLET_VERSION)
{
}

void btQuantizedBvh::deSerializeFloat(struct btQuantizedBvhFloatData& quantizedBvhFloatData)
{
	m_bvhAabbMax.deSerializeFloat(quantizedBvhFloatData.m_bvhAabbMax);
	m_bvhAabbMin.deSerializeFloat(quantizedBvhFloatData.m_bvhAabbMin);
	m_bvhQuantization.deSerializeFloat(quantizedBvhFloatData.m_bvhQuantization);

	m_curNodeIndex = quantizedBvhFloatData.m_curNodeIndex;
	m_useQuantization = quantizedBvhFloatData.m_useQuantization != 0;

	{
		int numElem = quantizedBvhFloatData.m_numContiguousLeafNodes;
		m_contiguousNodes.resize(numElem);

		if (numElem)
		{
			btOptimizedBvhNodeFloatData* memPtr = quantizedBvhFloatData.m_contiguousNodesPtr;

			for (int i = 0; i < numElem; i++, memPtr++)
			{
				m_contiguousNodes[i].m_aabbMaxOrg.deSerializeFloat(memPtr->m_aabbMaxOrg);
				m_contiguousNodes[i].m_aabbMinOrg.deSerializeFloat(memPtr->m_aabbMinOrg);
				m_contiguousNodes[i].m_escapeIndex = memPtr->m_escapeIndex;
				m_contiguousNodes[i].m_subPart = memPtr->m_subPart;
				m_contiguousNodes[i].m_triangleIndex = memPtr->m_triangleIndex;
			}
		}
	}

	{
		int numElem = quantizedBvhFloatData.m_numQuantizedContiguousNodes;
		m_quantizedContiguousNodes.resize(numElem);

		if (numElem)
		{
			btQuantizedBvhNodeData* memPtr = quantizedBvhFloatData.m_quantizedContiguousNodesPtr;
			for (int i = 0; i < numElem; i++, memPtr++)
			{
				m_quantizedContiguousNodes[i].m_escapeIndexOrTriangleIndex = memPtr->m_escapeIndexOrTriangleIndex;
				m_quantizedContiguousNodes[i].m_quantizedAabbMax[0] = memPtr->m_quantizedAabbMax[0];
				m_quantizedContiguousNodes[i].m_quantizedAabbMax[1] = memPtr->m_quantizedAabbMax[1];
				m_quantizedContiguousNodes[i].m_quantizedAabbMax[2] = memPtr->m_quantizedAabbMax[2];
				m_quantizedContiguousNodes[i].m_quantizedAabbMin[0] = memPtr->m_quantizedAabbMin[0];
				m_quantizedContiguousNodes[i].m_quantizedAabbMin[1] = memPtr->m_quantizedAabbMin[1];
				m_quantizedContiguousNodes[i].m_quantizedAabbMin[2] = memPtr->m_quantizedAabbMin[2];
			}
		}
	}

	m_traversalMode = btTraversalMode(quantizedBvhFloatData.m_traversalMode);

	{
		int numElem = quantizedBvhFloatData.m_numSubtreeHeaders;
		m_SubtreeHeaders.resize(numElem);
		if (numElem)
		{
			btBvhSubtreeInfoData* memPtr = quantizedBvhFloatData.m_subTreeInfoPtr;
			for (int i = 0; i < numElem; i++, memPtr++)
			{
				m_SubtreeHeaders[i].m_quantizedAabbMax[0] = memPtr->m_quantizedAabbMax[0];
				m_SubtreeHeaders[i].m_quantizedAabbMax[1] = memPtr->m_quantizedAabbMax[1];
				m_SubtreeHeaders[i].m_quantizedAabbMax[2] = memPtr->m_quantizedAabbMax[2];
				m_SubtreeHeaders[i].m_quantizedAabbMin[0] = memPtr->m_quantizedAabbMin[0];
				m_SubtreeHeaders[i].m_quantizedAabbMin[1] = memPtr->m_quantizedAabbMin[1];
				m_SubtreeHeaders[i].m_quantizedAabbMin[2] = memPtr->m_quantizedAabbMin[2];
				m_SubtreeHeaders[i].m_rootNodeIndex = memPtr->m_rootNodeIndex;
				m_SubtreeHeaders[i].m_subtreeSize = memPtr->m_subtreeSize;
			}
		}
	}
}

void btQuantizedBvh::deSerializeDouble(struct btQuantizedBvhDoubleData& quantizedBvhDoubleData)
{
	m_bvhAabbMax.deSerializeDouble(quantizedBvhDoubleData.m_bvhAabbMax);
	m_bvhAabbMin.deSerializeDouble(quantizedBvhDoubleData.m_bvhAabbMin);
	m_bvhQuantization.deSerializeDouble(quantizedBvhDoubleData.m_bvhQuantization);

	m_curNodeIndex = quantizedBvhDoubleData.m_curNodeIndex;
	m_useQuantization = quantizedBvhDoubleData.m_useQuantization != 0;

	{
		int numElem = quantizedBvhDoubleData.m_numContiguousLeafNodes;
		m_contiguousNodes.resize(numElem);

		if (numElem)
		{
			btOptimizedBvhNodeDoubleData* memPtr = quantizedBvhDoubleData.m_contiguousNodesPtr;

			for (int i = 0; i < numElem; i++, memPtr++)
			{
				m_contiguousNodes[i].m_aabbMaxOrg.deSerializeDouble(memPtr->m_aabbMaxOrg);
				m_contiguousNodes[i].m_aabbMinOrg.deSerializeDouble(memPtr->m_aabbMinOrg);
				m_contiguousNodes[i].m_escapeIndex = memPtr->m_escapeIndex;
				m_contiguousNodes[i].m_subPart = memPtr->m_subPart;
				m_contiguousNodes[i].m_triangleIndex = memPtr->m_triangleIndex;
			}
		}
	}

	{
		int numElem = quantizedBvhDoubleData.m_numQuantizedContiguousNodes;
		m_quantizedContiguousNodes.resize(numElem);

		if (numElem)
		{
			btQuantizedBvhNodeData* memPtr = quantizedBvhDoubleData.m_quantizedContiguousNodesPtr;
			for (int i = 0; i < numElem; i++, memPtr++)
			{
				m_quantizedContiguousNodes[i].m_escapeIndexOrTriangleIndex = memPtr->m_escapeIndexOrTriangleIndex;
				m_quantizedContiguousNodes[i].m_quantizedAabbMax[0] = memPtr->m_quantizedAabbMax[0];
				m_quantizedContiguousNodes[i].m_quantizedAabbMax[1] = memPtr->m_quantizedAabbMax[1];
				m_quantizedContiguousNodes[i].m_quantizedAabbMax[2] = memPtr->m_quantizedAabbMax[2];
				m_quantizedContiguousNodes[i].m_quantizedAabbMin[0] = memPtr->m_quantizedAabbMin[0];
				m_quantizedContiguousNodes[i].m_quantizedAabbMin[1] = memPtr->m_quantizedAabbMin[1];
				m_quantizedContiguousNodes[i].m_quantizedAabbMin[2] = memPtr->m_quantizedAabbMin[2];
			}
		}
	}

	m_traversalMode = btTraversalMode(quantizedBvhDoubleData.m_traversalMode);

	{
		int numElem = quantizedBvhDoubleData.m_numSubtreeHeaders;
		m_SubtreeHeaders.resize(numElem);
		if (numElem)
		{
			btBvhSubtreeInfoData* memPtr = quantizedBvhDoubleData.m_subTreeInfoPtr;
			for (int i = 0; i < numElem; i++, memPtr++)
			{
				m_SubtreeHeaders[i].m_quantizedAabbMax[0] = memPtr->m_quantizedAabbMax[0];
				m_SubtreeHeaders[i].m_quantizedAabbMax[1] = memPtr->m_quantizedAabbMax[1];
				m_SubtreeHeaders[i].m_quantizedAabbMax[2] = memPtr->m_quantizedAabbMax[2];
				m_SubtreeHeaders[i].m_quantizedAabbMin[0] = memPtr->m_quantizedAabbMin[0];
				m_SubtreeHeaders[i].m_quantizedAabbMin[1] = memPtr->m_quantizedAabbMin[1];
				m_SubtreeHeaders[i].m_quantizedAabbMin[2] = memPtr->m_quantizedAabbMin[2];
				m_SubtreeHeaders[i].m_rootNodeIndex = memPtr->m_rootNodeIndex;
				m_SubtreeHeaders[i].m_subtreeSize = memPtr->m_subtreeSize;
			}
		}
	}
}


const char* btQuantizedBvh::serialize(void* dataBuffer, btSerializer* serializer) const
{
	btQuantizedBvhData* quantizedData = (btQuantizedBvhData*)dataBuffer;

	m_bvhAabbMax.serialize(quantizedData->m_bvhAabbMax);
	m_bvhAabbMin.serialize(quantizedData->m_bvhAabbMin);
	m_bvhQuantization.serialize(quantizedData->m_bvhQuantization);

	quantizedData->m_curNodeIndex = m_curNodeIndex;
	quantizedData->m_useQuantization = m_useQuantization;

	quantizedData->m_numContiguousLeafNodes = m_contiguousNodes.size();
	quantizedData->m_contiguousNodesPtr = (btOptimizedBvhNodeData*)(m_contiguousNodes.size() ? serializer->getUniquePointer((void*)&m_contiguousNodes[0]) : 0);
	if (quantizedData->m_contiguousNodesPtr)
	{
		int sz = sizeof(btOptimizedBvhNodeData);
		int numElem = m_contiguousNodes.size();
		btChunk* chunk = serializer->allocate(sz, numElem);
		btOptimizedBvhNodeData* memPtr = (btOptimizedBvhNodeData*)chunk->m_oldPtr;
		for (int i = 0; i < numElem; i++, memPtr++)
		{
			m_contiguousNodes[i].m_aabbMaxOrg.serialize(memPtr->m_aabbMaxOrg);
			m_contiguousNodes[i].m_aabbMinOrg.serialize(memPtr->m_aabbMinOrg);
			memPtr->m_escapeIndex = m_contiguousNodes[i].m_escapeIndex;
			memPtr->m_subPart = m_contiguousNodes[i].m_subPart;
			memPtr->m_triangleIndex = m_contiguousNodes[i].m_triangleIndex;
			
			memset(memPtr->m_pad, 0, sizeof(memPtr->m_pad));
		}
		serializer->finalizeChunk(chunk, "btOptimizedBvhNodeData", BT_ARRAY_CODE, (void*)&m_contiguousNodes[0]);
	}

	quantizedData->m_numQuantizedContiguousNodes = m_quantizedContiguousNodes.size();
	
	quantizedData->m_quantizedContiguousNodesPtr = (btQuantizedBvhNodeData*)(m_quantizedContiguousNodes.size() ? serializer->getUniquePointer((void*)&m_quantizedContiguousNodes[0]) : 0);
	if (quantizedData->m_quantizedContiguousNodesPtr)
	{
		int sz = sizeof(btQuantizedBvhNodeData);
		int numElem = m_quantizedContiguousNodes.size();
		btChunk* chunk = serializer->allocate(sz, numElem);
		btQuantizedBvhNodeData* memPtr = (btQuantizedBvhNodeData*)chunk->m_oldPtr;
		for (int i = 0; i < numElem; i++, memPtr++)
		{
			memPtr->m_escapeIndexOrTriangleIndex = m_quantizedContiguousNodes[i].m_escapeIndexOrTriangleIndex;
			memPtr->m_quantizedAabbMax[0] = m_quantizedContiguousNodes[i].m_quantizedAabbMax[0];
			memPtr->m_quantizedAabbMax[1] = m_quantizedContiguousNodes[i].m_quantizedAabbMax[1];
			memPtr->m_quantizedAabbMax[2] = m_quantizedContiguousNodes[i].m_quantizedAabbMax[2];
			memPtr->m_quantizedAabbMin[0] = m_quantizedContiguousNodes[i].m_quantizedAabbMin[0];
			memPtr->m_quantizedAabbMin[1] = m_quantizedContiguousNodes[i].m_quantizedAabbMin[1];
			memPtr->m_quantizedAabbMin[2] = m_quantizedContiguousNodes[i].m_quantizedAabbMin[2];
		}
		serializer->finalizeChunk(chunk, "btQuantizedBvhNodeData", BT_ARRAY_CODE, (void*)&m_quantizedContiguousNodes[0]);
	}

	quantizedData->m_traversalMode = int(m_traversalMode);
	quantizedData->m_numSubtreeHeaders = m_SubtreeHeaders.size();

	quantizedData->m_subTreeInfoPtr = (btBvhSubtreeInfoData*)(m_SubtreeHeaders.size() ? serializer->getUniquePointer((void*)&m_SubtreeHeaders[0]) : 0);
	if (quantizedData->m_subTreeInfoPtr)
	{
		int sz = sizeof(btBvhSubtreeInfoData);
		int numElem = m_SubtreeHeaders.size();
		btChunk* chunk = serializer->allocate(sz, numElem);
		btBvhSubtreeInfoData* memPtr = (btBvhSubtreeInfoData*)chunk->m_oldPtr;
		for (int i = 0; i < numElem; i++, memPtr++)
		{
			memPtr->m_quantizedAabbMax[0] = m_SubtreeHeaders[i].m_quantizedAabbMax[0];
			memPtr->m_quantizedAabbMax[1] = m_SubtreeHeaders[i].m_quantizedAabbMax[1];
			memPtr->m_quantizedAabbMax[2] = m_SubtreeHeaders[i].m_quantizedAabbMax[2];
			memPtr->m_quantizedAabbMin[0] = m_SubtreeHeaders[i].m_quantizedAabbMin[0];
			memPtr->m_quantizedAabbMin[1] = m_SubtreeHeaders[i].m_quantizedAabbMin[1];
			memPtr->m_quantizedAabbMin[2] = m_SubtreeHeaders[i].m_quantizedAabbMin[2];

			memPtr->m_rootNodeIndex = m_SubtreeHeaders[i].m_rootNodeIndex;
			memPtr->m_subtreeSize = m_SubtreeHeaders[i].m_subtreeSize;
		}
		serializer->finalizeChunk(chunk, "btBvhSubtreeInfoData", BT_ARRAY_CODE, (void*)&m_SubtreeHeaders[0]);
	}
	return btQuantizedBvhDataName;
}
