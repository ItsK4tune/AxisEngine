



#include "gim_tri_collision.h"

#define TRI_LOCAL_EPSILON 0.000001f
#define MIN_EDGE_EDGE_DIS 0.00001f

class GIM_TRIANGLE_CALCULATION_CACHE
{
public:
	GREAL margin;
	btVector3 tu_vertices[3];
	btVector3 tv_vertices[3];
	btVector4 tu_plane;
	btVector4 tv_plane;
	btVector3 closest_point_u;
	btVector3 closest_point_v;
	btVector3 edge_edge_dir;
	btVector3 distances;
	GREAL du[4];
	GREAL du0du1;
	GREAL du0du2;
	GREAL dv[4];
	GREAL dv0dv1;
	GREAL dv0dv2;
	btVector3 temp_points[MAX_TRI_CLIPPING];
	btVector3 temp_points1[MAX_TRI_CLIPPING];
	btVector3 contact_points[MAX_TRI_CLIPPING];

	
	SIMD_FORCE_INLINE bool compute_intervals(
		const GREAL &D0,
		const GREAL &D1,
		const GREAL &D2,
		const GREAL &D0D1,
		const GREAL &D0D2,
		GREAL &scale_edge0,
		GREAL &scale_edge1,
		GUINT &edge_index0,
		GUINT &edge_index1)
	{
		if (D0D1 > 0.0f)
		{
			
			
			scale_edge0 = -D2 / (D0 - D2);
			scale_edge1 = -D1 / (D2 - D1);
			edge_index0 = 2;
			edge_index1 = 1;
		}
		else if (D0D2 > 0.0f)
		{
			
			scale_edge0 = -D0 / (D1 - D0);
			scale_edge1 = -D1 / (D2 - D1);
			edge_index0 = 0;
			edge_index1 = 1;
		}
		else if (D1 * D2 > 0.0f || D0 != 0.0f)
		{
			
			scale_edge0 = -D0 / (D1 - D0);
			scale_edge1 = -D2 / (D0 - D2);
			edge_index0 = 0;
			edge_index1 = 2;
		}
		else
		{
			return false;
		}
		return true;
	}

	
	
	SIMD_FORCE_INLINE GUINT clip_triangle(
		const btVector4 &tri_plane,
		const btVector3 *tripoints,
		const btVector3 *srcpoints,
		btVector3 *clip_points)
	{
		

		btVector4 edgeplane;

		EDGE_PLANE(tripoints[0], tripoints[1], tri_plane, edgeplane);

		GUINT clipped_count = PLANE_CLIP_TRIANGLE3D(
			edgeplane, srcpoints[0], srcpoints[1], srcpoints[2], temp_points);

		if (clipped_count == 0) return 0;

		

		EDGE_PLANE(tripoints[1], tripoints[2], tri_plane, edgeplane);

		clipped_count = PLANE_CLIP_POLYGON3D(
			edgeplane, temp_points, clipped_count, temp_points1);

		if (clipped_count == 0) return 0;

		

		EDGE_PLANE(tripoints[2], tripoints[0], tri_plane, edgeplane);

		clipped_count = PLANE_CLIP_POLYGON3D(
			edgeplane, temp_points1, clipped_count, clip_points);

		return clipped_count;

		
	}

	SIMD_FORCE_INLINE void sort_isect(
		GREAL &isect0, GREAL &isect1, GUINT &e0, GUINT &e1, btVector3 &vec0, btVector3 &vec1)
	{
		if (isect1 < isect0)
		{
			
			GIM_SWAP_NUMBERS(isect0, isect1);
			GIM_SWAP_NUMBERS(e0, e1);
			btVector3 tmp = vec0;
			vec0 = vec1;
			vec1 = tmp;
		}
	}

	
	
	SIMD_FORCE_INLINE GUINT cross_line_intersection_test()
	{
		
		edge_edge_dir = tu_plane.cross(tv_plane);
		GREAL Dlen;
		VEC_LENGTH(edge_edge_dir, Dlen);

		if (Dlen < 0.0001)
		{
			return 0;  
		}

		edge_edge_dir *= 1 / Dlen;  

		
		GUINT tu_e0, tu_e1;              
		GREAL tu_scale_e0, tu_scale_e1;  
		if (!compute_intervals(du[0], du[1], du[2],
							   du0du1, du0du2, tu_scale_e0, tu_scale_e1, tu_e0, tu_e1)) return 0;

		
		GUINT tv_e0, tv_e1;              
		GREAL tv_scale_e0, tv_scale_e1;  

		if (!compute_intervals(dv[0], dv[1], dv[2],
							   dv0dv1, dv0dv2, tv_scale_e0, tv_scale_e1, tv_e0, tv_e1)) return 0;

		
		btVector3 up_e0 = tu_vertices[tu_e0].lerp(tu_vertices[(tu_e0 + 1) % 3], tu_scale_e0);
		btVector3 up_e1 = tu_vertices[tu_e1].lerp(tu_vertices[(tu_e1 + 1) % 3], tu_scale_e1);

		btVector3 vp_e0 = tv_vertices[tv_e0].lerp(tv_vertices[(tv_e0 + 1) % 3], tv_scale_e0);
		btVector3 vp_e1 = tv_vertices[tv_e1].lerp(tv_vertices[(tv_e1 + 1) % 3], tv_scale_e1);

		
		GREAL isect_u[] = {up_e0.dot(edge_edge_dir), up_e1.dot(edge_edge_dir)};
		GREAL isect_v[] = {vp_e0.dot(edge_edge_dir), vp_e1.dot(edge_edge_dir)};

		sort_isect(isect_u[0], isect_u[1], tu_e0, tu_e1, up_e0, up_e1);
		sort_isect(isect_v[0], isect_v[1], tv_e0, tv_e1, vp_e0, vp_e1);

		const GREAL midpoint_u = 0.5f * (isect_u[0] + isect_u[1]);  
		const GREAL midpoint_v = 0.5f * (isect_v[0] + isect_v[1]);  

		if (midpoint_u < midpoint_v)
		{
			if (isect_u[1] >= isect_v[1])  
			{
				return 1;
			}
			else if (isect_v[0] <= isect_u[0])  
			{
				return 2;
			}
			
			closest_point_u = up_e1;
			closest_point_v = vp_e0;
			

			if (isect_u[1] + MIN_EDGE_EDGE_DIS < isect_v[0])  
			{
				SEGMENT_COLLISION(
					tu_vertices[tu_e1], tu_vertices[(tu_e1 + 1) % 3],
					tv_vertices[tv_e0], tv_vertices[(tv_e0 + 1) % 3],
					closest_point_u,
					closest_point_v);

				edge_edge_dir = closest_point_u - closest_point_v;
				VEC_LENGTH(edge_edge_dir, distances[2]);
				edge_edge_dir *= 1.0f / distances[2];  
			}
			else
			{
				distances[2] = isect_v[0] - isect_u[1];  
														 
			}
		}
		else
		{
			if (isect_v[1] >= isect_u[1])  
			{
				return 2;
			}
			else if (isect_u[0] <= isect_v[0])  
			{
				return 1;
			}
			
			closest_point_u = up_e0;
			closest_point_v = vp_e1;
			

			if (isect_v[1] + MIN_EDGE_EDGE_DIS < isect_u[0])  
			{
				SEGMENT_COLLISION(
					tu_vertices[tu_e0], tu_vertices[(tu_e0 + 1) % 3],
					tv_vertices[tv_e1], tv_vertices[(tv_e1 + 1) % 3],
					closest_point_u,
					closest_point_v);

				edge_edge_dir = closest_point_u - closest_point_v;
				VEC_LENGTH(edge_edge_dir, distances[2]);
				edge_edge_dir *= 1.0f / distances[2];  
			}
			else
			{
				distances[2] = isect_u[0] - isect_v[1];  
														 
			}
		}
		return 3;
	}

	
	SIMD_FORCE_INLINE bool triangle_collision(
		const btVector3 &u0,
		const btVector3 &u1,
		const btVector3 &u2,
		GREAL margin_u,
		const btVector3 &v0,
		const btVector3 &v1,
		const btVector3 &v2,
		GREAL margin_v,
		GIM_TRIANGLE_CONTACT_DATA &contacts)
	{
		margin = margin_u + margin_v;

		tu_vertices[0] = u0;
		tu_vertices[1] = u1;
		tu_vertices[2] = u2;

		tv_vertices[0] = v0;
		tv_vertices[1] = v1;
		tv_vertices[2] = v2;

		
		

		TRIANGLE_PLANE(tv_vertices[0], tv_vertices[1], tv_vertices[2], tv_plane);

		du[0] = DISTANCE_PLANE_POINT(tv_plane, tu_vertices[0]);
		du[1] = DISTANCE_PLANE_POINT(tv_plane, tu_vertices[1]);
		du[2] = DISTANCE_PLANE_POINT(tv_plane, tu_vertices[2]);

		du0du1 = du[0] * du[1];
		du0du2 = du[0] * du[2];

		if (du0du1 > 0.0f && du0du2 > 0.0f)  
		{
			if (du[0] < 0)  
			{
				distances[0] = GIM_MAX3(du[0], du[1], du[2]);
				distances[0] = -distances[0];
				if (distances[0] > margin) return false;  

				
				VEC_SWAP(tv_vertices[0], tv_vertices[1]);
				VEC_SCALE_4(tv_plane, -1.0f, tv_plane);
			}
			else
			{
				distances[0] = GIM_MIN3(du[0], du[1], du[2]);
				if (distances[0] > margin) return false;  
			}
		}
		else
		{
			
			distances[0] = (du[0] + du[1] + du[2]) / 3.0f;  

			if (distances[0] < 0.0f)
			{
				
				VEC_SWAP(tv_vertices[0], tv_vertices[1]);
				VEC_SCALE_4(tv_plane, -1.0f, tv_plane);

				distances[0] = GIM_MAX3(du[0], du[1], du[2]);
				distances[0] = -distances[0];
			}
			else
			{
				distances[0] = GIM_MIN3(du[0], du[1], du[2]);
			}
		}

		

		TRIANGLE_PLANE(tu_vertices[0], tu_vertices[1], tu_vertices[2], tu_plane);

		dv[0] = DISTANCE_PLANE_POINT(tu_plane, tv_vertices[0]);
		dv[1] = DISTANCE_PLANE_POINT(tu_plane, tv_vertices[1]);
		dv[2] = DISTANCE_PLANE_POINT(tu_plane, tv_vertices[2]);

		dv0dv1 = dv[0] * dv[1];
		dv0dv2 = dv[0] * dv[2];

		if (dv0dv1 > 0.0f && dv0dv2 > 0.0f)  
		{
			if (dv[0] < 0)  
			{
				distances[1] = GIM_MAX3(dv[0], dv[1], dv[2]);
				distances[1] = -distances[1];
				if (distances[1] > margin) return false;  

				
				VEC_SWAP(tu_vertices[0], tu_vertices[1]);
				VEC_SCALE_4(tu_plane, -1.0f, tu_plane);
			}
			else
			{
				distances[1] = GIM_MIN3(dv[0], dv[1], dv[2]);
				if (distances[1] > margin) return false;  
			}
		}
		else
		{
			
			distances[1] = (dv[0] + dv[1] + dv[2]) / 3.0f;  

			if (distances[1] < 0.0f)
			{
				
				VEC_SWAP(tu_vertices[0], tu_vertices[1]);
				VEC_SCALE_4(tu_plane, -1.0f, tu_plane);

				distances[1] = GIM_MAX3(dv[0], dv[1], dv[2]);
				distances[1] = -distances[1];
			}
			else
			{
				distances[1] = GIM_MIN3(dv[0], dv[1], dv[2]);
			}
		}

		GUINT bl;
		
		bl = 0;
		if (distances[0] < distances[1]) bl = 1;
		

		if (bl == 2)  
		{
			if (distances[2] > margin) return false;

			contacts.m_penetration_depth = -distances[2] + margin;
			contacts.m_points[0] = closest_point_v;
			contacts.m_point_count = 1;
			VEC_COPY(contacts.m_separating_normal, edge_edge_dir);

			return true;
		}

		

		GUINT point_count;
		
		if (bl == 0)  
		{
			point_count = clip_triangle(tv_plane, tv_vertices, tu_vertices, contact_points);
			if (point_count == 0) return false;
			contacts.merge_points(tv_plane, margin, contact_points, point_count);
		}
		else  
		{
			point_count = clip_triangle(tu_plane, tu_vertices, tv_vertices, contact_points);
			if (point_count == 0) return false;
			contacts.merge_points(tu_plane, margin, contact_points, point_count);
			contacts.m_separating_normal *= -1.f;
		}
		if (contacts.m_point_count == 0) return false;
		return true;
	}
};



bool GIM_TRIANGLE::collide_triangle_hard_test(
	const GIM_TRIANGLE &other,
	GIM_TRIANGLE_CONTACT_DATA &contact_data) const
{
	GIM_TRIANGLE_CALCULATION_CACHE calc_cache;
	return calc_cache.triangle_collision(
		m_vertices[0], m_vertices[1], m_vertices[2], m_margin,
		other.m_vertices[0], other.m_vertices[1], other.m_vertices[2], other.m_margin,
		contact_data);
}
