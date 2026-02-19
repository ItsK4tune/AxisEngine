#ifndef GIM_BOX_SET_STRUCT_H_INCLUDED
#define GIM_BOX_SET_STRUCT_H_INCLUDED




#include "LinearMath/btAlignedObjectArray.h"

#include "btBoxCollision.h"
#include "btTriangleShapeEx.h"
#include "gim_pair.h" 


struct GIM_BVH_DATA
{
	btAABB m_bound;
	int m_data;
};


class GIM_BVH_TREE_NODE
{
public:
	btAABB m_bound;

protected:
	int m_escapeIndexOrDataIndex;

public:
	GIM_BVH_TREE_NODE()
	{
		m_escapeIndexOrDataIndex = 0;
	}

	SIMD_FORCE_INLINE bool isLeafNode() const
	{
		
		return (m_escapeIndexOrDataIndex >= 0);
	}

	SIMD_FORCE_INLINE int getEscapeIndex() const
	{
		
		return -m_escapeIndexOrDataIndex;
	}

	SIMD_FORCE_INLINE void setEscapeIndex(int index)
	{
		m_escapeIndexOrDataIndex = -index;
	}

	SIMD_FORCE_INLINE int getDataIndex() const
	{
		

		return m_escapeIndexOrDataIndex;
	}

	SIMD_FORCE_INLINE void setDataIndex(int index)
	{
		m_escapeIndexOrDataIndex = index;
	}
};

#endif  
