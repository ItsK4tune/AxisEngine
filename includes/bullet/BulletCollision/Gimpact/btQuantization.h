#ifndef BT_GIMPACT_QUANTIZATION_H_INCLUDED
#define BT_GIMPACT_QUANTIZATION_H_INCLUDED




#include "LinearMath/btTransform.h"

SIMD_FORCE_INLINE void bt_calc_quantization_parameters(
	btVector3& outMinBound,
	btVector3& outMaxBound,
	btVector3& bvhQuantization,
	const btVector3& srcMinBound, const btVector3& srcMaxBound,
	btScalar quantizationMargin)
{
	
	btVector3 clampValue(quantizationMargin, quantizationMargin, quantizationMargin);
	outMinBound = srcMinBound - clampValue;
	outMaxBound = srcMaxBound + clampValue;
	btVector3 aabbSize = outMaxBound - outMinBound;
	bvhQuantization = btVector3(btScalar(65535.0),
								btScalar(65535.0),
								btScalar(65535.0)) /
					  aabbSize;
}

SIMD_FORCE_INLINE void bt_quantize_clamp(
	unsigned short* out,
	const btVector3& point,
	const btVector3& min_bound,
	const btVector3& max_bound,
	const btVector3& bvhQuantization)
{
	btVector3 clampedPoint(point);
	clampedPoint.setMax(min_bound);
	clampedPoint.setMin(max_bound);

	btVector3 v = (clampedPoint - min_bound) * bvhQuantization;
	out[0] = (unsigned short)(v.getX() + 0.5f);
	out[1] = (unsigned short)(v.getY() + 0.5f);
	out[2] = (unsigned short)(v.getZ() + 0.5f);
}

SIMD_FORCE_INLINE btVector3 bt_unquantize(
	const unsigned short* vecIn,
	const btVector3& offset,
	const btVector3& bvhQuantization)
{
	btVector3 vecOut;
	vecOut.setValue(
		(btScalar)(vecIn[0]) / (bvhQuantization.getX()),
		(btScalar)(vecIn[1]) / (bvhQuantization.getY()),
		(btScalar)(vecIn[2]) / (bvhQuantization.getZ()));
	vecOut += offset;
	return vecOut;
}

#endif  
