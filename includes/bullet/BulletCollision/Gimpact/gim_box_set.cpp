


#include "gim_box_set.h"

GUINT GIM_BOX_TREE::_calc_splitting_axis(
	gim_array<GIM_AABB_DATA>& primitive_boxes, GUINT startIndex, GUINT endIndex)
{
	GUINT i;

	btVector3 means(btScalar(0.), btScalar(0.), btScalar(0.));
	btVector3 variance(btScalar(0.), btScalar(0.), btScalar(0.));
	GUINT numIndices = endIndex - startIndex;

	for (i = startIndex; i < endIndex; i++)
	{
		btVector3 center = btScalar(0.5) * (primitive_boxes[i].m_bound.m_max +
											primitive_boxes[i].m_bound.m_min);
		means += center;
	}
	means *= (btScalar(1.) / (btScalar)numIndices);

	for (i = startIndex; i < endIndex; i++)
	{
		btVector3 center = btScalar(0.5) * (primitive_boxes[i].m_bound.m_max +
											primitive_boxes[i].m_bound.m_min);
		btVector3 diff2 = center - means;
		diff2 = diff2 * diff2;
		variance += diff2;
	}
	variance *= (btScalar(1.) / ((btScalar)numIndices - 1));

	return variance.maxAxis();
}

GUINT GIM_BOX_TREE::_sort_and_calc_splitting_index(
	gim_array<GIM_AABB_DATA>& primitive_boxes, GUINT startIndex,
	GUINT endIndex, GUINT splitAxis)
{
	GUINT i;
	GUINT splitIndex = startIndex;
	GUINT numIndices = endIndex - startIndex;

	
	btScalar splitValue = 0.0f;
	for (i = startIndex; i < endIndex; i++)
	{
		splitValue += 0.5f * (primitive_boxes[i].m_bound.m_max[splitAxis] +
							  primitive_boxes[i].m_bound.m_min[splitAxis]);
	}
	splitValue /= (btScalar)numIndices;

	
	for (i = startIndex; i < endIndex; i++)
	{
		btScalar center = 0.5f * (primitive_boxes[i].m_bound.m_max[splitAxis] +
								  primitive_boxes[i].m_bound.m_min[splitAxis]);
		if (center > splitValue)
		{
			
			primitive_boxes.swap(i, splitIndex);
			splitIndex++;
		}
	}

	
	
	
	

	
	

	
	GUINT rangeBalancedIndices = numIndices / 3;
	bool unbalanced = ((splitIndex <= (startIndex + rangeBalancedIndices)) || (splitIndex >= (endIndex - 1 - rangeBalancedIndices)));

	if (unbalanced)
	{
		splitIndex = startIndex + (numIndices >> 1);
	}

	btAssert(!((splitIndex == startIndex) || (splitIndex == (endIndex))));

	return splitIndex;
}

void GIM_BOX_TREE::_build_sub_tree(gim_array<GIM_AABB_DATA>& primitive_boxes, GUINT startIndex, GUINT endIndex)
{
	GUINT current_index = m_num_nodes++;

	btAssert((endIndex - startIndex) > 0);

	if ((endIndex - startIndex) == 1)  
	{
		m_node_array[current_index].m_left = 0;
		m_node_array[current_index].m_right = 0;
		m_node_array[current_index].m_escapeIndex = 0;

		m_node_array[current_index].m_bound = primitive_boxes[startIndex].m_bound;
		m_node_array[current_index].m_data = primitive_boxes[startIndex].m_data;
		return;
	}

	

	GUINT splitIndex;

	
	m_node_array[current_index].m_bound.invalidate();
	for (splitIndex = startIndex; splitIndex < endIndex; splitIndex++)
	{
		m_node_array[current_index].m_bound.merge(primitive_boxes[splitIndex].m_bound);
	}

	

	
	splitIndex = _calc_splitting_axis(primitive_boxes, startIndex, endIndex);

	splitIndex = _sort_and_calc_splitting_index(
		primitive_boxes, startIndex, endIndex, splitIndex);

	
	m_node_array[current_index].m_left = m_num_nodes;
	
	_build_sub_tree(primitive_boxes, startIndex, splitIndex);

	
	m_node_array[current_index].m_right = m_num_nodes;

	
	_build_sub_tree(primitive_boxes, splitIndex, endIndex);

	
	m_node_array[current_index].m_escapeIndex = m_num_nodes - current_index;
}


void GIM_BOX_TREE::build_tree(
	gim_array<GIM_AABB_DATA>& primitive_boxes)
{
	
	m_num_nodes = 0;
	
	m_node_array.resize(primitive_boxes.size() * 2);

	_build_sub_tree(primitive_boxes, 0, primitive_boxes.size());
}
