#ifndef GIM_QUANTIZED_SET_H_INCLUDED
#define GIM_QUANTIZED_SET_H_INCLUDED




#include "btGImpactBvh.h"
#include "btQuantization.h"
#include "btGImpactQuantizedBvhStructs.h"

class GIM_QUANTIZED_BVH_NODE_ARRAY : public btAlignedObjectArray<BT_QUANTIZED_BVH_NODE>
{
};


class btQuantizedBvhTree
{
protected:
	int m_num_nodes;
	GIM_QUANTIZED_BVH_NODE_ARRAY m_node_array;
	btAABB m_global_bound;
	btVector3 m_bvhQuantization;

protected:
	void calc_quantization(GIM_BVH_DATA_ARRAY& primitive_boxes, btScalar boundMargin = btScalar(1.0));

	int _sort_and_calc_splitting_index(
		GIM_BVH_DATA_ARRAY& primitive_boxes,
		int startIndex, int endIndex, int splitAxis);

	int _calc_splitting_axis(GIM_BVH_DATA_ARRAY& primitive_boxes, int startIndex, int endIndex);

	void _build_sub_tree(GIM_BVH_DATA_ARRAY& primitive_boxes, int startIndex, int endIndex);

public:
	btQuantizedBvhTree()
	{
		m_num_nodes = 0;
	}

	
	
	void build_tree(GIM_BVH_DATA_ARRAY& primitive_boxes);

	SIMD_FORCE_INLINE void quantizePoint(
		unsigned short* quantizedpoint, const btVector3& point) const
	{
		bt_quantize_clamp(quantizedpoint, point, m_global_bound.m_min, m_global_bound.m_max, m_bvhQuantization);
	}

	SIMD_FORCE_INLINE bool testQuantizedBoxOverlapp(
		int node_index,
		unsigned short* quantizedMin, unsigned short* quantizedMax) const
	{
		return m_node_array[node_index].testQuantizedBoxOverlapp(quantizedMin, quantizedMax);
	}

	SIMD_FORCE_INLINE void clearNodes()
	{
		m_node_array.clear();
		m_num_nodes = 0;
	}

	
	SIMD_FORCE_INLINE int getNodeCount() const
	{
		return m_num_nodes;
	}

	
	SIMD_FORCE_INLINE bool isLeafNode(int nodeindex) const
	{
		return m_node_array[nodeindex].isLeafNode();
	}

	SIMD_FORCE_INLINE int getNodeData(int nodeindex) const
	{
		return m_node_array[nodeindex].getDataIndex();
	}

	SIMD_FORCE_INLINE void getNodeBound(int nodeindex, btAABB& bound) const
	{
		bound.m_min = bt_unquantize(
			m_node_array[nodeindex].m_quantizedAabbMin,
			m_global_bound.m_min, m_bvhQuantization);

		bound.m_max = bt_unquantize(
			m_node_array[nodeindex].m_quantizedAabbMax,
			m_global_bound.m_min, m_bvhQuantization);
	}

	SIMD_FORCE_INLINE void setNodeBound(int nodeindex, const btAABB& bound)
	{
		bt_quantize_clamp(m_node_array[nodeindex].m_quantizedAabbMin,
						  bound.m_min,
						  m_global_bound.m_min,
						  m_global_bound.m_max,
						  m_bvhQuantization);

		bt_quantize_clamp(m_node_array[nodeindex].m_quantizedAabbMax,
						  bound.m_max,
						  m_global_bound.m_min,
						  m_global_bound.m_max,
						  m_bvhQuantization);
	}

	SIMD_FORCE_INLINE int getLeftNode(int nodeindex) const
	{
		return nodeindex + 1;
	}

	SIMD_FORCE_INLINE int getRightNode(int nodeindex) const
	{
		if (m_node_array[nodeindex + 1].isLeafNode()) return nodeindex + 2;
		return nodeindex + 1 + m_node_array[nodeindex + 1].getEscapeIndex();
	}

	SIMD_FORCE_INLINE int getEscapeNodeIndex(int nodeindex) const
	{
		return m_node_array[nodeindex].getEscapeIndex();
	}

	SIMD_FORCE_INLINE const BT_QUANTIZED_BVH_NODE* get_node_pointer(int index = 0) const
	{
		return &m_node_array[index];
	}

	
};



class btGImpactQuantizedBvh
{
protected:
	btQuantizedBvhTree m_box_tree;
	btPrimitiveManagerBase* m_primitive_manager;

protected:
	
	void refit();

public:
	
	btGImpactQuantizedBvh()
	{
		m_primitive_manager = NULL;
	}

	
	btGImpactQuantizedBvh(btPrimitiveManagerBase* primitive_manager)
	{
		m_primitive_manager = primitive_manager;
	}

	SIMD_FORCE_INLINE btAABB getGlobalBox() const
	{
		btAABB totalbox;
		getNodeBound(0, totalbox);
		return totalbox;
	}

	SIMD_FORCE_INLINE void setPrimitiveManager(btPrimitiveManagerBase* primitive_manager)
	{
		m_primitive_manager = primitive_manager;
	}

	SIMD_FORCE_INLINE btPrimitiveManagerBase* getPrimitiveManager() const
	{
		return m_primitive_manager;
	}

	
	

	
	SIMD_FORCE_INLINE void update()
	{
		refit();
	}

	
	void buildSet();

	
	bool boxQuery(const btAABB& box, btAlignedObjectArray<int>& collided_results) const;

	
	SIMD_FORCE_INLINE bool boxQueryTrans(const btAABB& box,
										 const btTransform& transform, btAlignedObjectArray<int>& collided_results) const
	{
		btAABB transbox = box;
		transbox.appy_transform(transform);
		return boxQuery(transbox, collided_results);
	}

	
	bool rayQuery(
		const btVector3& ray_dir, const btVector3& ray_origin,
		btAlignedObjectArray<int>& collided_results) const;

	
	SIMD_FORCE_INLINE bool hasHierarchy() const
	{
		return true;
	}

	
	SIMD_FORCE_INLINE bool isTrimesh() const
	{
		return m_primitive_manager->is_trimesh();
	}

	
	SIMD_FORCE_INLINE int getNodeCount() const
	{
		return m_box_tree.getNodeCount();
	}

	
	SIMD_FORCE_INLINE bool isLeafNode(int nodeindex) const
	{
		return m_box_tree.isLeafNode(nodeindex);
	}

	SIMD_FORCE_INLINE int getNodeData(int nodeindex) const
	{
		return m_box_tree.getNodeData(nodeindex);
	}

	SIMD_FORCE_INLINE void getNodeBound(int nodeindex, btAABB& bound) const
	{
		m_box_tree.getNodeBound(nodeindex, bound);
	}

	SIMD_FORCE_INLINE void setNodeBound(int nodeindex, const btAABB& bound)
	{
		m_box_tree.setNodeBound(nodeindex, bound);
	}

	SIMD_FORCE_INLINE int getLeftNode(int nodeindex) const
	{
		return m_box_tree.getLeftNode(nodeindex);
	}

	SIMD_FORCE_INLINE int getRightNode(int nodeindex) const
	{
		return m_box_tree.getRightNode(nodeindex);
	}

	SIMD_FORCE_INLINE int getEscapeNodeIndex(int nodeindex) const
	{
		return m_box_tree.getEscapeNodeIndex(nodeindex);
	}

	SIMD_FORCE_INLINE void getNodeTriangle(int nodeindex, btPrimitiveTriangle& triangle) const
	{
		m_primitive_manager->get_primitive_triangle(getNodeData(nodeindex), triangle);
	}

	SIMD_FORCE_INLINE const BT_QUANTIZED_BVH_NODE* get_node_pointer(int index = 0) const
	{
		return m_box_tree.get_node_pointer(index);
	}

#ifdef TRI_COLLISION_PROFILING
	static float getAverageTreeCollisionTime();
#endif  

	static void find_collision(const btGImpactQuantizedBvh* boxset1, const btTransform& trans1,
							   const btGImpactQuantizedBvh* boxset2, const btTransform& trans2,
							   btPairSet& collision_pairs);
};

#endif  
