#ifndef GIM_CLIP_POLYGON_H_INCLUDED
#define GIM_CLIP_POLYGON_H_INCLUDED





class DISTANCE_PLANE_3D_FUNC
{
public:
	template <typename CLASS_POINT, typename CLASS_PLANE>
	inline GREAL operator()(const CLASS_PLANE& plane, const CLASS_POINT& point)
	{
		return DISTANCE_PLANE_POINT(plane, point);
	}
};

template <typename CLASS_POINT>
SIMD_FORCE_INLINE void PLANE_CLIP_POLYGON_COLLECT(
	const CLASS_POINT& point0,
	const CLASS_POINT& point1,
	GREAL dist0,
	GREAL dist1,
	CLASS_POINT* clipped,
	GUINT& clipped_count)
{
	GUINT _prevclassif = (dist0 > G_EPSILON);
	GUINT _classif = (dist1 > G_EPSILON);
	if (_classif != _prevclassif)
	{
		GREAL blendfactor = -dist0 / (dist1 - dist0);
		VEC_BLEND(clipped[clipped_count], point0, point1, blendfactor);
		clipped_count++;
	}
	if (!_classif)
	{
		VEC_COPY(clipped[clipped_count], point1);
		clipped_count++;
	}
}



template <typename CLASS_POINT, typename CLASS_PLANE, typename DISTANCE_PLANE_FUNC>
SIMD_FORCE_INLINE GUINT PLANE_CLIP_POLYGON_GENERIC(
	const CLASS_PLANE& plane,
	const CLASS_POINT* polygon_points,
	GUINT polygon_point_count,
	CLASS_POINT* clipped, DISTANCE_PLANE_FUNC distance_func)
{
	GUINT clipped_count = 0;

	
	GREAL firstdist = distance_func(plane, polygon_points[0]);
	;
	if (!(firstdist > G_EPSILON))
	{
		VEC_COPY(clipped[clipped_count], polygon_points[0]);
		clipped_count++;
	}

	GREAL olddist = firstdist;
	for (GUINT _i = 1; _i < polygon_point_count; _i++)
	{
		GREAL dist = distance_func(plane, polygon_points[_i]);

		PLANE_CLIP_POLYGON_COLLECT(
			polygon_points[_i - 1], polygon_points[_i],
			olddist,
			dist,
			clipped,
			clipped_count);

		olddist = dist;
	}

	

	PLANE_CLIP_POLYGON_COLLECT(
		polygon_points[polygon_point_count - 1], polygon_points[0],
		olddist,
		firstdist,
		clipped,
		clipped_count);

	return clipped_count;
}



template <typename CLASS_POINT, typename CLASS_PLANE, typename DISTANCE_PLANE_FUNC>
SIMD_FORCE_INLINE GUINT PLANE_CLIP_TRIANGLE_GENERIC(
	const CLASS_PLANE& plane,
	const CLASS_POINT& point0,
	const CLASS_POINT& point1,
	const CLASS_POINT& point2,
	CLASS_POINT* clipped, DISTANCE_PLANE_FUNC distance_func)
{
	GUINT clipped_count = 0;

	
	GREAL firstdist = distance_func(plane, point0);
	;
	if (!(firstdist > G_EPSILON))
	{
		VEC_COPY(clipped[clipped_count], point0);
		clipped_count++;
	}

	
	GREAL olddist = firstdist;
	GREAL dist = distance_func(plane, point1);

	PLANE_CLIP_POLYGON_COLLECT(
		point0, point1,
		olddist,
		dist,
		clipped,
		clipped_count);

	olddist = dist;

	
	dist = distance_func(plane, point2);

	PLANE_CLIP_POLYGON_COLLECT(
		point1, point2,
		olddist,
		dist,
		clipped,
		clipped_count);
	olddist = dist;

	
	PLANE_CLIP_POLYGON_COLLECT(
		point2, point0,
		olddist,
		firstdist,
		clipped,
		clipped_count);

	return clipped_count;
}

template <typename CLASS_POINT, typename CLASS_PLANE>
SIMD_FORCE_INLINE GUINT PLANE_CLIP_POLYGON3D(
	const CLASS_PLANE& plane,
	const CLASS_POINT* polygon_points,
	GUINT polygon_point_count,
	CLASS_POINT* clipped)
{
	return PLANE_CLIP_POLYGON_GENERIC<CLASS_POINT, CLASS_PLANE>(plane, polygon_points, polygon_point_count, clipped, DISTANCE_PLANE_3D_FUNC());
}

template <typename CLASS_POINT, typename CLASS_PLANE>
SIMD_FORCE_INLINE GUINT PLANE_CLIP_TRIANGLE3D(
	const CLASS_PLANE& plane,
	const CLASS_POINT& point0,
	const CLASS_POINT& point1,
	const CLASS_POINT& point2,
	CLASS_POINT* clipped)
{
	return PLANE_CLIP_TRIANGLE_GENERIC<CLASS_POINT, CLASS_PLANE>(plane, point0, point1, point2, clipped, DISTANCE_PLANE_3D_FUNC());
}

#endif  
