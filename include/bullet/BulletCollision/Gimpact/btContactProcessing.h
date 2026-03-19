#ifndef BT_CONTACT_H_INCLUDED
#define BT_CONTACT_H_INCLUDED




#include "LinearMath/btTransform.h"
#include "LinearMath/btAlignedObjectArray.h"
#include "btTriangleShapeEx.h"
#include "btContactProcessingStructs.h"

class btContactArray : public btAlignedObjectArray<GIM_CONTACT>
{
public:
	btContactArray()
	{
		reserve(64);
	}

	SIMD_FORCE_INLINE void push_contact(
		const btVector3 &point, const btVector3 &normal,
		btScalar depth, int feature1, int feature2)
	{
		push_back(GIM_CONTACT(point, normal, depth, feature1, feature2));
	}

	SIMD_FORCE_INLINE void push_triangle_contacts(
		const GIM_TRIANGLE_CONTACT &tricontact,
		int feature1, int feature2)
	{
		for (int i = 0; i < tricontact.m_point_count; i++)
		{
			push_contact(
				tricontact.m_points[i],
				tricontact.m_separating_normal,
				tricontact.m_penetration_depth, feature1, feature2);
		}
	}

	void merge_contacts(const btContactArray &contacts, bool normal_contact_average = true);

	void merge_contacts_unique(const btContactArray &contacts);
};

#endif  
