#ifndef GIM_CONTACT_H_INCLUDED
#define GIM_CONTACT_H_INCLUDED



#include "gim_geometry.h"
#include "gim_radixsort.h"
#include "gim_array.h"


#ifndef NORMAL_CONTACT_AVERAGE
#define NORMAL_CONTACT_AVERAGE 1
#endif

#ifndef CONTACT_DIFF_EPSILON
#define CONTACT_DIFF_EPSILON 0.00001f
#endif

#ifndef BT_CONTACT_H_STRUCTS_INCLUDED






class GIM_CONTACT
{
public:
	btVector3 m_point;
	btVector3 m_normal;
	GREAL m_depth;     
	GREAL m_distance;  
	GUINT m_feature1;  
	GUINT m_feature2;  
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
		m_point = contact.m_point;
		m_normal = contact.m_normal;
		m_depth = contact.m_depth;
		m_feature1 = contact.m_feature1;
		m_feature2 = contact.m_feature2;
	}

	GIM_CONTACT(const btVector3 &point, const btVector3 &normal,
				GREAL depth, GUINT feature1, GUINT feature2) : m_point(point),
															   m_normal(normal),
															   m_depth(depth),
															   m_feature1(feature1),
															   m_feature2(feature2)
	{
	}

	
	SIMD_FORCE_INLINE GUINT calc_key_contact() const
	{
		GINT _coords[] = {
			(GINT)(m_point[0] * 1000.0f + 1.0f),
			(GINT)(m_point[1] * 1333.0f),
			(GINT)(m_point[2] * 2133.0f + 3.0f)};
		GUINT _hash = 0;
		GUINT *_uitmp = (GUINT *)(&_coords[0]);
		_hash = *_uitmp;
		_uitmp++;
		_hash += (*_uitmp) << 4;
		_uitmp++;
		_hash += (*_uitmp) << 8;
		return _hash;
	}

	SIMD_FORCE_INLINE void interpolate_normals(btVector3 *normals, GUINT normal_count)
	{
		btVector3 vec_sum(m_normal);
		for (GUINT i = 0; i < normal_count; i++)
		{
			vec_sum += normals[i];
		}

		GREAL vec_sum_len = vec_sum.length2();
		if (vec_sum_len < CONTACT_DIFF_EPSILON) return;

		GIM_INV_SQRT(vec_sum_len, vec_sum_len);  

		m_normal = vec_sum * vec_sum_len;
	}
};

#endif

class gim_contact_array : public gim_array<GIM_CONTACT>
{
public:
	gim_contact_array() : gim_array<GIM_CONTACT>(64)
	{
	}

	SIMD_FORCE_INLINE void push_contact(const btVector3 &point, const btVector3 &normal,
										GREAL depth, GUINT feature1, GUINT feature2)
	{
		push_back_mem();
		GIM_CONTACT &newele = back();
		newele.m_point = point;
		newele.m_normal = normal;
		newele.m_depth = depth;
		newele.m_feature1 = feature1;
		newele.m_feature2 = feature2;
	}

	SIMD_FORCE_INLINE void push_triangle_contacts(
		const GIM_TRIANGLE_CONTACT_DATA &tricontact,
		GUINT feature1, GUINT feature2)
	{
		for (GUINT i = 0; i < tricontact.m_point_count; i++)
		{
			push_back_mem();
			GIM_CONTACT &newele = back();
			newele.m_point = tricontact.m_points[i];
			newele.m_normal = tricontact.m_separating_normal;
			newele.m_depth = tricontact.m_penetration_depth;
			newele.m_feature1 = feature1;
			newele.m_feature2 = feature2;
		}
	}

	void merge_contacts(const gim_contact_array &contacts, bool normal_contact_average = true);
	void merge_contacts_unique(const gim_contact_array &contacts);
};

#endif  
