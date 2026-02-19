#ifndef BT_BASIC_GEOMETRY_OPERATIONS_H_INCLUDED
#define BT_BASIC_GEOMETRY_OPERATIONS_H_INCLUDED




#include "btBoxCollision.h"

#define PLANEDIREPSILON 0.0000001f
#define PARALELENORMALS 0.000001f

#define BT_CLAMP(number, minval, maxval) (number < minval ? minval : (number > maxval ? maxval : number))


SIMD_FORCE_INLINE void bt_edge_plane(const btVector3 &e1, const btVector3 &e2, const btVector3 &normal, btVector4 &plane)
{
	btVector3 planenormal = (e2 - e1).cross(normal);
	planenormal.normalize();
	plane.setValue(planenormal[0], planenormal[1], planenormal[2], e2.dot(planenormal));
}




SIMD_FORCE_INLINE void bt_closest_point_on_segment(
	btVector3 &cp, const btVector3 &v,
	const btVector3 &e1, const btVector3 &e2)
{
	btVector3 n = e2 - e1;
	cp = v - e1;
	btScalar _scalar = cp.dot(n) / n.dot(n);
	if (_scalar < 0.0f)
	{
		cp = e1;
	}
	else if (_scalar > 1.0f)
	{
		cp = e2;
	}
	else
	{
		cp = _scalar * n + e1;
	}
}




SIMD_FORCE_INLINE int bt_line_plane_collision(
	const btVector4 &plane,
	const btVector3 &vDir,
	const btVector3 &vPoint,
	btVector3 &pout,
	btScalar &tparam,
	btScalar tmin, btScalar tmax)
{
	btScalar _dotdir = vDir.dot(plane);

	if (btFabs(_dotdir) < PLANEDIREPSILON)
	{
		tparam = tmax;
		return 0;
	}

	btScalar _dis = bt_distance_point_plane(plane, vPoint);
	char returnvalue = _dis < 0.0f ? 2 : 1;
	tparam = -_dis / _dotdir;

	if (tparam < tmin)
	{
		returnvalue = 0;
		tparam = tmin;
	}
	else if (tparam > tmax)
	{
		returnvalue = 0;
		tparam = tmax;
	}
	pout = tparam * vDir + vPoint;
	return returnvalue;
}


SIMD_FORCE_INLINE void bt_segment_collision(
	const btVector3 &vA1,
	const btVector3 &vA2,
	const btVector3 &vB1,
	const btVector3 &vB2,
	btVector3 &vPointA,
	btVector3 &vPointB)
{
	btVector3 AD = vA2 - vA1;
	btVector3 BD = vB2 - vB1;
	btVector3 N = AD.cross(BD);
	btScalar tp = N.length2();

	btVector4 _M;  

	if (tp < SIMD_EPSILON)  
	{
		
		bool invert_b_order = false;
		_M[0] = vB1.dot(AD);
		_M[1] = vB2.dot(AD);

		if (_M[0] > _M[1])
		{
			invert_b_order = true;
			BT_SWAP_NUMBERS(_M[0], _M[1]);
		}
		_M[2] = vA1.dot(AD);
		_M[3] = vA2.dot(AD);
		
		N[0] = (_M[0] + _M[1]) * 0.5f;
		N[1] = (_M[2] + _M[3]) * 0.5f;

		if (N[0] < N[1])
		{
			if (_M[1] < _M[2])
			{
				vPointB = invert_b_order ? vB1 : vB2;
				vPointA = vA1;
			}
			else if (_M[1] < _M[3])
			{
				vPointB = invert_b_order ? vB1 : vB2;
				bt_closest_point_on_segment(vPointA, vPointB, vA1, vA2);
			}
			else
			{
				vPointA = vA2;
				bt_closest_point_on_segment(vPointB, vPointA, vB1, vB2);
			}
		}
		else
		{
			if (_M[3] < _M[0])
			{
				vPointB = invert_b_order ? vB2 : vB1;
				vPointA = vA2;
			}
			else if (_M[3] < _M[1])
			{
				vPointA = vA2;
				bt_closest_point_on_segment(vPointB, vPointA, vB1, vB2);
			}
			else
			{
				vPointB = invert_b_order ? vB1 : vB2;
				bt_closest_point_on_segment(vPointA, vPointB, vA1, vA2);
			}
		}
		return;
	}

	N = N.cross(BD);
	_M.setValue(N[0], N[1], N[2], vB1.dot(N));

	
	bt_line_plane_collision(_M, AD, vA1, vPointA, tp, btScalar(0), btScalar(1));

	
	vPointB = vPointA - vB1;
	tp = vPointB.dot(BD);
	tp /= BD.dot(BD);
	tp = BT_CLAMP(tp, 0.0f, 1.0f);

	vPointB = tp * BD + vB1;
}

#endif  
