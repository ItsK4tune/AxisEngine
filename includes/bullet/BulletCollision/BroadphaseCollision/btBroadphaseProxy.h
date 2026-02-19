

#ifndef BT_BROADPHASE_PROXY_H
#define BT_BROADPHASE_PROXY_H

#include "LinearMath/btScalar.h"  
#include "LinearMath/btVector3.h"
#include "LinearMath/btAlignedAllocator.h"





enum BroadphaseNativeTypes
{
	
	BOX_SHAPE_PROXYTYPE,
	TRIANGLE_SHAPE_PROXYTYPE,
	TETRAHEDRAL_SHAPE_PROXYTYPE,
	CONVEX_TRIANGLEMESH_SHAPE_PROXYTYPE,
	CONVEX_HULL_SHAPE_PROXYTYPE,
	CONVEX_POINT_CLOUD_SHAPE_PROXYTYPE,
	CUSTOM_POLYHEDRAL_SHAPE_TYPE,
	
	IMPLICIT_CONVEX_SHAPES_START_HERE,
	SPHERE_SHAPE_PROXYTYPE,
	MULTI_SPHERE_SHAPE_PROXYTYPE,
	CAPSULE_SHAPE_PROXYTYPE,
	CONE_SHAPE_PROXYTYPE,
	CONVEX_SHAPE_PROXYTYPE,
	CYLINDER_SHAPE_PROXYTYPE,
	UNIFORM_SCALING_SHAPE_PROXYTYPE,
	MINKOWSKI_SUM_SHAPE_PROXYTYPE,
	MINKOWSKI_DIFFERENCE_SHAPE_PROXYTYPE,
	BOX_2D_SHAPE_PROXYTYPE,
	CONVEX_2D_SHAPE_PROXYTYPE,
	CUSTOM_CONVEX_SHAPE_TYPE,
	
	CONCAVE_SHAPES_START_HERE,
	
	TRIANGLE_MESH_SHAPE_PROXYTYPE,
	SCALED_TRIANGLE_MESH_SHAPE_PROXYTYPE,
	
	FAST_CONCAVE_MESH_PROXYTYPE,
	
	TERRAIN_SHAPE_PROXYTYPE,
	
	GIMPACT_SHAPE_PROXYTYPE,
	
	MULTIMATERIAL_TRIANGLE_MESH_PROXYTYPE,

	EMPTY_SHAPE_PROXYTYPE,
	STATIC_PLANE_PROXYTYPE,
	CUSTOM_CONCAVE_SHAPE_TYPE,
	SDF_SHAPE_PROXYTYPE = CUSTOM_CONCAVE_SHAPE_TYPE,
	CONCAVE_SHAPES_END_HERE,

	COMPOUND_SHAPE_PROXYTYPE,

	SOFTBODY_SHAPE_PROXYTYPE,
	HFFLUID_SHAPE_PROXYTYPE,
	HFFLUID_BUOYANT_CONVEX_SHAPE_PROXYTYPE,
	INVALID_SHAPE_PROXYTYPE,

	MAX_BROADPHASE_COLLISION_TYPES

};



ATTRIBUTE_ALIGNED16(struct)
btBroadphaseProxy
{
	BT_DECLARE_ALIGNED_ALLOCATOR();

	
	enum CollisionFilterGroups
	{
		DefaultFilter = 1,
		StaticFilter = 2,
		KinematicFilter = 4,
		DebrisFilter = 8,
		SensorTrigger = 16,
		CharacterFilter = 32,
		AllFilter = -1  
	};

	
	void* m_clientObject;
	int m_collisionFilterGroup;
	int m_collisionFilterMask;

	int m_uniqueId;  

	btVector3 m_aabbMin;
	btVector3 m_aabbMax;

	SIMD_FORCE_INLINE int getUid() const
	{
		return m_uniqueId;
	}

	
	btBroadphaseProxy() : m_clientObject(0)
	{
	}

	btBroadphaseProxy(const btVector3& aabbMin, const btVector3& aabbMax, void* userPtr, int collisionFilterGroup, int collisionFilterMask)
		: m_clientObject(userPtr),
		  m_collisionFilterGroup(collisionFilterGroup),
		  m_collisionFilterMask(collisionFilterMask),
		  m_aabbMin(aabbMin),
		  m_aabbMax(aabbMax)
	{
	}

	static SIMD_FORCE_INLINE bool isPolyhedral(int proxyType)
	{
		return (proxyType < IMPLICIT_CONVEX_SHAPES_START_HERE);
	}

	static SIMD_FORCE_INLINE bool isConvex(int proxyType)
	{
		return (proxyType < CONCAVE_SHAPES_START_HERE);
	}

	static SIMD_FORCE_INLINE bool isNonMoving(int proxyType)
	{
		return (isConcave(proxyType) && !(proxyType == GIMPACT_SHAPE_PROXYTYPE));
	}

	static SIMD_FORCE_INLINE bool isConcave(int proxyType)
	{
		return ((proxyType > CONCAVE_SHAPES_START_HERE) &&
				(proxyType < CONCAVE_SHAPES_END_HERE));
	}
	static SIMD_FORCE_INLINE bool isCompound(int proxyType)
	{
		return (proxyType == COMPOUND_SHAPE_PROXYTYPE);
	}

	static SIMD_FORCE_INLINE bool isSoftBody(int proxyType)
	{
		return (proxyType == SOFTBODY_SHAPE_PROXYTYPE);
	}

	static SIMD_FORCE_INLINE bool isInfinite(int proxyType)
	{
		return (proxyType == STATIC_PLANE_PROXYTYPE);
	}

	static SIMD_FORCE_INLINE bool isConvex2d(int proxyType)
	{
		return (proxyType == BOX_2D_SHAPE_PROXYTYPE) || (proxyType == CONVEX_2D_SHAPE_PROXYTYPE);
	}
};

class btCollisionAlgorithm;

struct btBroadphaseProxy;



ATTRIBUTE_ALIGNED16(struct)
btBroadphasePair
{
	btBroadphasePair()
		: m_pProxy0(0),
		  m_pProxy1(0),
		  m_algorithm(0),
		  m_internalInfo1(0)
	{
	}

	BT_DECLARE_ALIGNED_ALLOCATOR();

	btBroadphasePair(btBroadphaseProxy & proxy0, btBroadphaseProxy & proxy1)
	{
		
		if (proxy0.m_uniqueId < proxy1.m_uniqueId)
		{
			m_pProxy0 = &proxy0;
			m_pProxy1 = &proxy1;
		}
		else
		{
			m_pProxy0 = &proxy1;
			m_pProxy1 = &proxy0;
		}

		m_algorithm = 0;
		m_internalInfo1 = 0;
	}

	btBroadphaseProxy* m_pProxy0;
	btBroadphaseProxy* m_pProxy1;

	mutable btCollisionAlgorithm* m_algorithm;
	union {
		void* m_internalInfo1;
		int m_internalTmpValue;
	};  
};



class btBroadphasePairSortPredicate
{
public:
	bool operator()(const btBroadphasePair& a, const btBroadphasePair& b) const
	{
		const int uidA0 = a.m_pProxy0 ? a.m_pProxy0->m_uniqueId : -1;
		const int uidB0 = b.m_pProxy0 ? b.m_pProxy0->m_uniqueId : -1;
		const int uidA1 = a.m_pProxy1 ? a.m_pProxy1->m_uniqueId : -1;
		const int uidB1 = b.m_pProxy1 ? b.m_pProxy1->m_uniqueId : -1;

		return uidA0 > uidB0 ||
			   (a.m_pProxy0 == b.m_pProxy0 && uidA1 > uidB1) ||
			   (a.m_pProxy0 == b.m_pProxy0 && a.m_pProxy1 == b.m_pProxy1 && a.m_algorithm > b.m_algorithm);
	}
};

SIMD_FORCE_INLINE bool operator==(const btBroadphasePair& a, const btBroadphasePair& b)
{
	return (a.m_pProxy0 == b.m_pProxy0) && (a.m_pProxy1 == b.m_pProxy1);
}

#endif  
