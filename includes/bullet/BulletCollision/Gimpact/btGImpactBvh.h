#ifndef BT_GIMPACT_BVH_H_INCLUDED
#define BT_GIMPACT_BVH_H_INCLUDED




#include "LinearMath/btAlignedObjectArray.h"

#include "btBoxCollision.h"
#include "btTriangleShapeEx.h"
#include "btGImpactBvhStructs.h"


class btPairSet : public btAlignedObjectArray<GIM_PAIR>
{
public:
	btPairSet()
	{
		reserve(32);
	}
	inline void push_pair(int index1, int index2)
	{
		push_back(GIM_PAIR(index1, index2));
	}

	inline void push_pair_inv(int index1, int index2)
	{
		push_back(GIM_PAIR(index2, index1));
	}
};

class GIM_BVH_DATA_ARRAY : public btAlignedObjectArray<GIM_BVH_DATA>
{
};

class GIM_BVH_TREE_NODE_ARRAY : public btAlignedObjectArray<GIM_BVH_TREE_NODE>
{
};


class btBvhTree
{
protected:
	int m_num_nodes;
	GIM_BVH_TREE_NODE_ARRAY m_node_array;

protected:
	int _sort_and_calc_splitting_index(
		GIM_BVH_DATA_ARRAY& primitive_boxes,
		int startIndex, int endIndex, int splitAxis);

	int _calc_splitting_axis(GIM_BVH_DATA_ARRAY& primitive_boxes, int startIndex, int endIndex);

	void _build_sub_tree(GIM_BVH_DATA_ARRAY& primitive_boxes, int startIndex, int endIndex);

public:
	btBvhTree()
	{
		m_num_nodes = 0;
	}

	
	
	void build_tree(GIM_BVH_DATA_ARRAY& primitive_boxes);

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
		bound = m_node_array[nodeindex].m_bound;
	}

	SIMD_FORCE_INLINE void setNodeBound(int nodeindex, const btAABB& bound)
	{
		m_node_array[nodeindex].m_bound = bound;
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

	SIMD_FORCE_INLINE const GIM_BVH_TREE_NODE* get_node_pointer(int index = 0) const
	{
		return &m_node_array[index];
	}

	
};



class btPrimitiveManagerBase
{
public:
	virtual ~btPrimitiveManagerBase() {}

	
	virtual bool is_trimesh() const = 0;
	virtual int get_primitive_count() const = 0;
	virtual void get_primitive_box(int prim_index, btAABB& primbox) const = 0;
	
	virtual void get_primitive_triangle(int prim_index, btPrimitiveTriangle& triangle) const = 0;
};



class btGImpactBvh
{
protected:
	btBvhTree m_box_tree;
	btPrimitiveManagerBase* m_primitive_manager;

protected:
	
	void refit();

public:
	
	btGImpactBvh()
	{
		m_primitive_manager = NULL;
	}

	
	btGImpactBvh(btPrimitiveManagerBase* primitive_manager)
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

	SIMD_FORCE_INLINE const GIM_BVH_TREE_NODE* get_node_pointer(int index = 0) const
	{
		return m_box_tree.get_node_pointer(index);
	}

#ifdef TRI_COLLISION_PROFILING
	static float getAverageTreeCollisionTime();
#endif  

	static void find_collision(btGImpactBvh* boxset1, const btTransform& trans1,
							   btGImpactBvh* boxset2, const btTransform& trans2,
							   btPairSet& collision_pairs);
};

#endif  
