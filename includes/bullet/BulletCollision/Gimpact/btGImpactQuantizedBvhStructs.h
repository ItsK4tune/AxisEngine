#ifndef GIM_QUANTIZED_SET_STRUCTS_H_INCLUDED
#define GIM_QUANTIZED_SET_STRUCTS_H_INCLUDED




#include "btGImpactBvh.h"
#include "btQuantization.h"



ATTRIBUTE_ALIGNED16(struct)
BT_QUANTIZED_BVH_NODE
{
	
	unsigned short int m_quantizedAabbMin[3];
	unsigned short int m_quantizedAabbMax[3];
	
	int m_escapeIndexOrDataIndex;

	BT_QUANTIZED_BVH_NODE()
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

	SIMD_FORCE_INLINE bool testQuantizedBoxOverlapp(
		unsigned short* quantizedMin, unsigned short* quantizedMax) const
	{
		if (m_quantizedAabbMin[0] > quantizedMax[0] ||
			m_quantizedAabbMax[0] < quantizedMin[0] ||
			m_quantizedAabbMin[1] > quantizedMax[1] ||
			m_quantizedAabbMax[1] < quantizedMin[1] ||
			m_quantizedAabbMin[2] > quantizedMax[2] ||
			m_quantizedAabbMax[2] < quantizedMin[2])
		{
			return false;
		}
		return true;
	}
};

#endif  
