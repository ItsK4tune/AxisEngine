


#ifndef GIMPACT_MASS_UTIL_H
#define GIMPACT_MASS_UTIL_H

#include "LinearMath/btTransform.h"

SIMD_FORCE_INLINE btVector3 gim_inertia_add_transformed(
	const btVector3& source_inertia, const btVector3& added_inertia, const btTransform& transform)
{
	btMatrix3x3 rotatedTensor = transform.getBasis().scaled(added_inertia) * transform.getBasis().transpose();

	btScalar x2 = transform.getOrigin()[0];
	x2 *= x2;
	btScalar y2 = transform.getOrigin()[1];
	y2 *= y2;
	btScalar z2 = transform.getOrigin()[2];
	z2 *= z2;

	btScalar ix = rotatedTensor[0][0] * (y2 + z2);
	btScalar iy = rotatedTensor[1][1] * (x2 + z2);
	btScalar iz = rotatedTensor[2][2] * (x2 + y2);

	return btVector3(source_inertia[0] + ix, source_inertia[1] + iy, source_inertia[2] + iz);
}

SIMD_FORCE_INLINE btVector3 gim_get_point_inertia(const btVector3& point, btScalar mass)
{
	btScalar x2 = point[0] * point[0];
	btScalar y2 = point[1] * point[1];
	btScalar z2 = point[2] * point[2];
	return btVector3(mass * (y2 + z2), mass * (x2 + z2), mass * (x2 + y2));
}

#endif  
