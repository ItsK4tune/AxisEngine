#ifndef BT_CONTACT_H_STRUCTS_INCLUDED
#define BT_CONTACT_H_STRUCTS_INCLUDED




#include "LinearMath/btTransform.h"
#include "LinearMath/btAlignedObjectArray.h"
#include "btTriangleShapeEx.h"


#define NORMAL_CONTACT_AVERAGE 1

#define CONTACT_DIFF_EPSILON 0.00001f



class GIM_CONTACT
{
public:
	btVector3 m_point;
	btVector3 m_normal;
	btScalar m_depth;     
	btScalar m_distance;  
	int m_feature1;       
	int m_feature2;       
public:
	GIM_CONTACT()
	{
	}

	GIM_CONTACT(const GIM_CONTACT &contact) : m_point(contact.m_point),
											  m_normal(contact.m_normal),
											  m_depth(contact.m_depth),
											  m_feature1(contact.m_feature1),
											  m_feature2(contact.m_feature2)
	{
	}

	GIM_CONTACT(const btVector3 &point, const btVector3 &normal,
				btScalar depth, int feature1, int feature2) : m_point(point),
															  m_normal(normal),
															  m_depth(depth),
															  m_feature1(feature1),
															  m_feature2(feature2)
	{
	}

	
	SIMD_FORCE_INLINE unsigned int calc_key_contact() const
	{
		int _coords[] = {
			(int)(m_point[0] * 1000.0f + 1.0f),
			(int)(m_point[1] * 1333.0f),
			(int)(m_point[2] * 2133.0f + 3.0f)};
		unsigned int _hash = 0;
		unsigned int *_uitmp = (unsigned int *)(&_coords[0]);
		_hash = *_uitmp;
		_uitmp++;
		_hash += (*_uitmp) << 4;
		_uitmp++;
		_hash += (*_uitmp) << 8;
		return _hash;
	}

	SIMD_FORCE_INLINE void interpolate_normals(btVector3 *normals, int normal_count)
	{
		btVector3 vec_sum(m_normal);
		for (int i = 0; i < normal_count; i++)
		{
			vec_sum += normals[i];
		}

		btScalar vec_sum_len = vec_sum.length2();
		if (vec_sum_len < CONTACT_DIFF_EPSILON) return;

		

		m_normal = vec_sum / btSqrt(vec_sum_len);
	}
};

#endif  
