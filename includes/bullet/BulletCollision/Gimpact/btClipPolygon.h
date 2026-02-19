#ifndef BT_CLIP_POLYGON_H_INCLUDED
#define BT_CLIP_POLYGON_H_INCLUDED




#include "LinearMath/btTransform.h"
#include "LinearMath/btGeometryUtil.h"

SIMD_FORCE_INLINE btScalar bt_distance_point_plane(const btVector4 &plane, const btVector3 &point)
{
	return point.dot(plane) - plane[3];
}


SIMD_FORCE_INLINE void bt_vec_blend(btVector3 &vr, const btVector3 &va, const btVector3 &vb, btScalar blend_factor)
{
	vr = (1 - blend_factor) * va + blend_factor * vb;
}


SIMD_FORCE_INLINE void bt_plane_clip_polygon_collect(
	const btVector3 &point0,
	const btVector3 &point1,
	btScalar dist0,
	btScalar dist1,
	btVector3 *clipped,
	int &clipped_count)
{
	bool _prevclassif = (dist0 > SIMD_EPSILON);
	bool _classif = (dist1 > SIMD_EPSILON);
	if (_classif != _prevclassif)
	{
		btScalar blendfactor = -dist0 / (dist1 - dist0);
		bt_vec_blend(clipped[clipped_count], point0, point1, blendfactor);
		clipped_count++;
	}
	if (!_classif)
	{
		clipped[clipped_count] = point1;
		clipped_count++;
	}
}



SIMD_FORCE_INLINE int bt_plane_clip_polygon(
	const btVector4 &plane,
	const btVector3 *polygon_points,
	int polygon_point_count,
	btVector3 *clipped)
{
	int clipped_count = 0;

	
	btScalar firstdist = bt_distance_point_plane(plane, polygon_points[0]);
	;
	if (!(firstdist > SIMD_EPSILON))
	{
		clipped[clipped_count] = polygon_points[0];
		clipped_count++;
	}

	btScalar olddist = firstdist;
	for (int i = 1; i < polygon_point_count; i++)
	{
		btScalar dist = bt_distance_point_plane(plane, polygon_points[i]);

		bt_plane_clip_polygon_collect(
			polygon_points[i - 1], polygon_points[i],
			olddist,
			dist,
			clipped,
			clipped_count);

		olddist = dist;
	}

	

	bt_plane_clip_polygon_collect(
		polygon_points[polygon_point_count - 1], polygon_points[0],
		olddist,
		firstdist,
		clipped,
		clipped_count);

	return clipped_count;
}



SIMD_FORCE_INLINE int bt_plane_clip_triangle(
	const btVector4 &plane,
	const btVector3 &point0,
	const btVector3 &point1,
	const btVector3 &point2,
	btVector3 *clipped  
)
{
	int clipped_count = 0;

	
	btScalar firstdist = bt_distance_point_plane(plane, point0);
	;
	if (!(firstdist > SIMD_EPSILON))
	{
		clipped[clipped_count] = point0;
		clipped_count++;
	}

	
	btScalar olddist = firstdist;
	btScalar dist = bt_distance_point_plane(plane, point1);

	bt_plane_clip_polygon_collect(
		point0, point1,
		olddist,
		dist,
		clipped,
		clipped_count);

	olddist = dist;

	
	dist = bt_distance_point_plane(plane, point2);

	bt_plane_clip_polygon_collect(
		point1, point2,
		olddist,
		dist,
		clipped,
		clipped_count);
	olddist = dist;

	
	bt_plane_clip_polygon_collect(
		point2, point0,
		olddist,
		firstdist,
		clipped,
		clipped_count);

	return clipped_count;
}

#endif  
