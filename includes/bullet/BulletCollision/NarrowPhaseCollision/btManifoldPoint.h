

#ifndef BT_MANIFOLD_CONTACT_POINT_H
#define BT_MANIFOLD_CONTACT_POINT_H

#include "LinearMath/btVector3.h"
#include "LinearMath/btTransformUtil.h"

#ifdef PFX_USE_FREE_VECTORMATH
#include "physics_effects/base_level/solver/pfx_constraint_row.h"
typedef sce::PhysicsEffects::PfxConstraintRow btConstraintRow;
#else

ATTRIBUTE_ALIGNED16(struct)
btConstraintRow
{
	btScalar m_normal[3];
	btScalar m_rhs;
	btScalar m_jacDiagInv;
	btScalar m_lowerLimit;
	btScalar m_upperLimit;
	btScalar m_accumImpulse;
};
typedef btConstraintRow PfxConstraintRow;
#endif  

enum btContactPointFlags
{
	BT_CONTACT_FLAG_LATERAL_FRICTION_INITIALIZED = 1,
	BT_CONTACT_FLAG_HAS_CONTACT_CFM = 2,
	BT_CONTACT_FLAG_HAS_CONTACT_ERP = 4,
	BT_CONTACT_FLAG_CONTACT_STIFFNESS_DAMPING = 8,
	BT_CONTACT_FLAG_FRICTION_ANCHOR = 16,
};



class btManifoldPoint
{
public:
	btManifoldPoint()
		: m_userPersistentData(0),
		  m_contactPointFlags(0),
		  m_appliedImpulse(0.f),
		  m_prevRHS(0.f),
		  m_appliedImpulseLateral1(0.f),
		  m_appliedImpulseLateral2(0.f),
		  m_contactMotion1(0.f),
		  m_contactMotion2(0.f),
		  m_contactCFM(0.f),
		  m_contactERP(0.f),
		  m_frictionCFM(0.f),
		  m_lifeTime(0)
	{
	}

	btManifoldPoint(const btVector3& pointA, const btVector3& pointB,
					const btVector3& normal,
					btScalar distance) : m_localPointA(pointA),
										 m_localPointB(pointB),
										 m_positionWorldOnB(0,0,0),
										 m_positionWorldOnA(0,0,0),
										 m_normalWorldOnB(normal),
										 m_distance1(distance),
										 m_combinedFriction(btScalar(0.)),
										 m_combinedRollingFriction(btScalar(0.)),
										 m_combinedSpinningFriction(btScalar(0.)),
										 m_combinedRestitution(btScalar(0.)),
										 m_partId0(-1),
										 m_partId1(-1),
										 m_index0(-1),
										 m_index1(-1),
										 m_userPersistentData(0),
										 m_contactPointFlags(0),
										 m_appliedImpulse(0.f),
										 m_prevRHS(0.f),
										 m_appliedImpulseLateral1(0.f),
										 m_appliedImpulseLateral2(0.f),
										 m_contactMotion1(0.f),
										 m_contactMotion2(0.f),
										 m_contactCFM(0.f),
										 m_contactERP(0.f),
										 m_frictionCFM(0.f),
										 m_lifeTime(0),
										 m_lateralFrictionDir1(0,0,0),
										 m_lateralFrictionDir2(0,0,0)
	{
	}

	btVector3 m_localPointA;
	btVector3 m_localPointB;
	btVector3 m_positionWorldOnB;
	
	btVector3 m_positionWorldOnA;
	btVector3 m_normalWorldOnB;

	btScalar m_distance1;
	btScalar m_combinedFriction;
	btScalar m_combinedRollingFriction;   
	btScalar m_combinedSpinningFriction;  
	btScalar m_combinedRestitution;

	
	int m_partId0;
	int m_partId1;
	int m_index0;
	int m_index1;

	mutable void* m_userPersistentData;
	
	int m_contactPointFlags;

	btScalar m_appliedImpulse;
	btScalar m_prevRHS;
	btScalar m_appliedImpulseLateral1;
	btScalar m_appliedImpulseLateral2;
	btScalar m_contactMotion1;
	btScalar m_contactMotion2;

	union {
		btScalar m_contactCFM;
		btScalar m_combinedContactStiffness1;
	};

	union {
		btScalar m_contactERP;
		btScalar m_combinedContactDamping1;
	};

	btScalar m_frictionCFM;

	int m_lifeTime;  

	btVector3 m_lateralFrictionDir1;
	btVector3 m_lateralFrictionDir2;

	btScalar getDistance() const
	{
		return m_distance1;
	}
	int getLifeTime() const
	{
		return m_lifeTime;
	}

	const btVector3& getPositionWorldOnA() const
	{
		return m_positionWorldOnA;
		
	}

	const btVector3& getPositionWorldOnB() const
	{
		return m_positionWorldOnB;
	}

	void setDistance(btScalar dist)
	{
		m_distance1 = dist;
	}

	
	btScalar getAppliedImpulse() const
	{
		return m_appliedImpulse;
	}
};

#endif  
