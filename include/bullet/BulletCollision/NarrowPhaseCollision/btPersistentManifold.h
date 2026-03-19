

#ifndef BT_PERSISTENT_MANIFOLD_H
#define BT_PERSISTENT_MANIFOLD_H

#include "LinearMath/btVector3.h"
#include "LinearMath/btTransform.h"
#include "btManifoldPoint.h"
class btCollisionObject;
#include "LinearMath/btAlignedAllocator.h"

struct btCollisionResult;
struct btCollisionObjectDoubleData;
struct btCollisionObjectFloatData;


extern btScalar gContactBreakingThreshold;

#ifndef SWIG
class btPersistentManifold;

typedef bool (*ContactDestroyedCallback)(void* userPersistentData);
typedef bool (*ContactProcessedCallback)(btManifoldPoint& cp, void* body0, void* body1);
typedef void (*ContactStartedCallback)(btPersistentManifold* const& manifold);
typedef void (*ContactEndedCallback)(btPersistentManifold* const& manifold);
extern ContactDestroyedCallback gContactDestroyedCallback;
extern ContactProcessedCallback gContactProcessedCallback;
extern ContactStartedCallback gContactStartedCallback;
extern ContactEndedCallback gContactEndedCallback;
#endif  


enum btContactManifoldTypes
{
	MIN_CONTACT_MANIFOLD_TYPE = 1024,
	BT_PERSISTENT_MANIFOLD_TYPE
};

#define MANIFOLD_CACHE_SIZE 4










ATTRIBUTE_ALIGNED16(class)
btPersistentManifold : public btTypedObject
{
	btManifoldPoint m_pointCache[MANIFOLD_CACHE_SIZE];

	
	const btCollisionObject* m_body0;
	const btCollisionObject* m_body1;

	int m_cachedPoints;

	btScalar m_contactBreakingThreshold;
	btScalar m_contactProcessingThreshold;

	
	int sortCachedPoints(const btManifoldPoint& pt);

	int findContactPoint(const btManifoldPoint* unUsed, int numUnused, const btManifoldPoint& pt);

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	int m_companionIdA;
	int m_companionIdB;

	int m_index1a;

	btPersistentManifold();

	btPersistentManifold(const btCollisionObject* body0, const btCollisionObject* body1, int, btScalar contactBreakingThreshold, btScalar contactProcessingThreshold)
		: btTypedObject(BT_PERSISTENT_MANIFOLD_TYPE),
		  m_body0(body0),
		  m_body1(body1),
		  m_cachedPoints(0),
		  m_contactBreakingThreshold(contactBreakingThreshold),
		  m_contactProcessingThreshold(contactProcessingThreshold),
		  m_companionIdA(0),
		  m_companionIdB(0),
		  m_index1a(0)
	{
	}

	SIMD_FORCE_INLINE const btCollisionObject* getBody0() const { return m_body0; }
	SIMD_FORCE_INLINE const btCollisionObject* getBody1() const { return m_body1; }

	void setBodies(const btCollisionObject* body0, const btCollisionObject* body1)
	{
		m_body0 = body0;
		m_body1 = body1;
	}

	void clearUserCache(btManifoldPoint & pt);

#ifdef DEBUG_PERSISTENCY
	void DebugPersistency();
#endif  

	SIMD_FORCE_INLINE int getNumContacts() const
	{
		return m_cachedPoints;
	}
	
	void setNumContacts(int cachedPoints)
	{
		m_cachedPoints = cachedPoints;
	}

	SIMD_FORCE_INLINE const btManifoldPoint& getContactPoint(int index) const
	{
		btAssert(index < m_cachedPoints);
		return m_pointCache[index];
	}

	SIMD_FORCE_INLINE btManifoldPoint& getContactPoint(int index)
	{
		btAssert(index < m_cachedPoints);
		return m_pointCache[index];
	}

	
	btScalar getContactBreakingThreshold() const;

	btScalar getContactProcessingThreshold() const
	{
		return m_contactProcessingThreshold;
	}

	void setContactBreakingThreshold(btScalar contactBreakingThreshold)
	{
		m_contactBreakingThreshold = contactBreakingThreshold;
	}

	void setContactProcessingThreshold(btScalar contactProcessingThreshold)
	{
		m_contactProcessingThreshold = contactProcessingThreshold;
	}

	int getCacheEntry(const btManifoldPoint& newPoint) const;

	int addManifoldPoint(const btManifoldPoint& newPoint, bool isPredictive = false);

	void removeContactPoint(int index)
	{
		clearUserCache(m_pointCache[index]);

		int lastUsedIndex = getNumContacts() - 1;
		
		if (index != lastUsedIndex)
		{
			m_pointCache[index] = m_pointCache[lastUsedIndex];
			
			m_pointCache[lastUsedIndex].m_userPersistentData = 0;
			m_pointCache[lastUsedIndex].m_appliedImpulse = 0.f;
			m_pointCache[lastUsedIndex].m_prevRHS = 0.f;
			m_pointCache[lastUsedIndex].m_contactPointFlags = 0;
			m_pointCache[lastUsedIndex].m_appliedImpulseLateral1 = 0.f;
			m_pointCache[lastUsedIndex].m_appliedImpulseLateral2 = 0.f;
			m_pointCache[lastUsedIndex].m_lifeTime = 0;
		}

		btAssert(m_pointCache[lastUsedIndex].m_userPersistentData == 0);
		m_cachedPoints--;

		if (gContactEndedCallback && m_cachedPoints == 0)
		{
			gContactEndedCallback(this);
		}
	}
	void replaceContactPoint(const btManifoldPoint& newPoint, int insertIndex)
	{
		btAssert(validContactDistance(newPoint));

#define MAINTAIN_PERSISTENCY 1
#ifdef MAINTAIN_PERSISTENCY
		int lifeTime = m_pointCache[insertIndex].getLifeTime();
		btScalar appliedImpulse = m_pointCache[insertIndex].m_appliedImpulse;
		btScalar prevRHS = m_pointCache[insertIndex].m_prevRHS;
		btScalar appliedLateralImpulse1 = m_pointCache[insertIndex].m_appliedImpulseLateral1;
		btScalar appliedLateralImpulse2 = m_pointCache[insertIndex].m_appliedImpulseLateral2;

		bool replacePoint = true;
		
		
		if (newPoint.m_contactPointFlags & BT_CONTACT_FLAG_FRICTION_ANCHOR)
		{
			
			
			
			
			btScalar mu = m_pointCache[insertIndex].m_combinedFriction;
			btScalar eps = 0;  
			btScalar a = appliedLateralImpulse1 * appliedLateralImpulse1 + appliedLateralImpulse2 * appliedLateralImpulse2;
			btScalar b = eps + mu * appliedImpulse;
			b = b * b;
			replacePoint = (a) > (b);
		}

		if (replacePoint)
		{
			btAssert(lifeTime >= 0);
			void* cache = m_pointCache[insertIndex].m_userPersistentData;

			m_pointCache[insertIndex] = newPoint;
			m_pointCache[insertIndex].m_userPersistentData = cache;
			m_pointCache[insertIndex].m_appliedImpulse = appliedImpulse;
			m_pointCache[insertIndex].m_prevRHS = prevRHS;
			m_pointCache[insertIndex].m_appliedImpulseLateral1 = appliedLateralImpulse1;
			m_pointCache[insertIndex].m_appliedImpulseLateral2 = appliedLateralImpulse2;
		}

		m_pointCache[insertIndex].m_lifeTime = lifeTime;
#else
		clearUserCache(m_pointCache[insertIndex]);
		m_pointCache[insertIndex] = newPoint;

#endif
	}

	bool validContactDistance(const btManifoldPoint& pt) const
	{
		return pt.m_distance1 <= getContactBreakingThreshold();
	}
	
	void refreshContactPoints(const btTransform& trA, const btTransform& trB);

	SIMD_FORCE_INLINE void clearManifold()
	{
		int i;
		for (i = 0; i < m_cachedPoints; i++)
		{
			clearUserCache(m_pointCache[i]);
		}

		if (gContactEndedCallback && m_cachedPoints)
		{
			gContactEndedCallback(this);
		}
		m_cachedPoints = 0;
	}

	int calculateSerializeBufferSize() const;
	const char* serialize(const class btPersistentManifold* manifold, void* dataBuffer, class btSerializer* serializer) const;
	void deSerialize(const struct btPersistentManifoldDoubleData* manifoldDataPtr);
	void deSerialize(const struct btPersistentManifoldFloatData* manifoldDataPtr);
};



struct btPersistentManifoldDoubleData
{
	btVector3DoubleData m_pointCacheLocalPointA[4];
	btVector3DoubleData m_pointCacheLocalPointB[4];
	btVector3DoubleData m_pointCachePositionWorldOnA[4];
	btVector3DoubleData m_pointCachePositionWorldOnB[4];
	btVector3DoubleData m_pointCacheNormalWorldOnB[4];
	btVector3DoubleData	m_pointCacheLateralFrictionDir1[4];
	btVector3DoubleData	m_pointCacheLateralFrictionDir2[4];
	double m_pointCacheDistance[4];
	double m_pointCacheAppliedImpulse[4];
	double m_pointCachePrevRHS[4];
	 double m_pointCacheCombinedFriction[4];
	double m_pointCacheCombinedRollingFriction[4];
	double m_pointCacheCombinedSpinningFriction[4];
	double m_pointCacheCombinedRestitution[4];
	int	m_pointCachePartId0[4];
	int	m_pointCachePartId1[4];
	int	m_pointCacheIndex0[4];
	int	m_pointCacheIndex1[4];
	int m_pointCacheContactPointFlags[4];
	double m_pointCacheAppliedImpulseLateral1[4];
	double m_pointCacheAppliedImpulseLateral2[4];
	double m_pointCacheContactMotion1[4];
	double m_pointCacheContactMotion2[4];
	double m_pointCacheContactCFM[4];
	double m_pointCacheCombinedContactStiffness1[4];
	double m_pointCacheContactERP[4];
	double m_pointCacheCombinedContactDamping1[4];
	double m_pointCacheFrictionCFM[4];
	int m_pointCacheLifeTime[4];

	int m_numCachedPoints;
	int m_companionIdA;
	int m_companionIdB;
	int m_index1a;

	int m_objectType;
	double	m_contactBreakingThreshold;
	double	m_contactProcessingThreshold;
	int m_padding;

	btCollisionObjectDoubleData *m_body0;
	btCollisionObjectDoubleData *m_body1;
};


struct btPersistentManifoldFloatData
{
	btVector3FloatData m_pointCacheLocalPointA[4];
	btVector3FloatData m_pointCacheLocalPointB[4];
	btVector3FloatData m_pointCachePositionWorldOnA[4];
	btVector3FloatData m_pointCachePositionWorldOnB[4];
	btVector3FloatData m_pointCacheNormalWorldOnB[4];
	btVector3FloatData	m_pointCacheLateralFrictionDir1[4];
	btVector3FloatData	m_pointCacheLateralFrictionDir2[4];
	float m_pointCacheDistance[4];
	float m_pointCacheAppliedImpulse[4];
	float m_pointCachePrevRHS[4];
	float m_pointCacheCombinedFriction[4];
	float m_pointCacheCombinedRollingFriction[4];
	float m_pointCacheCombinedSpinningFriction[4];
	float m_pointCacheCombinedRestitution[4];
	int	m_pointCachePartId0[4];
	int	m_pointCachePartId1[4];
	int	m_pointCacheIndex0[4];
	int	m_pointCacheIndex1[4];
	int m_pointCacheContactPointFlags[4];
	float m_pointCacheAppliedImpulseLateral1[4];
	float m_pointCacheAppliedImpulseLateral2[4];
	float m_pointCacheContactMotion1[4];
	float m_pointCacheContactMotion2[4];
	float m_pointCacheContactCFM[4];
	float m_pointCacheCombinedContactStiffness1[4];
	float m_pointCacheContactERP[4];
	float m_pointCacheCombinedContactDamping1[4];
	float m_pointCacheFrictionCFM[4];
	int m_pointCacheLifeTime[4];

	int m_numCachedPoints;
	int m_companionIdA;
	int m_companionIdB;
	int m_index1a;

	int m_objectType;
	float	m_contactBreakingThreshold;
	float	m_contactProcessingThreshold;
	int m_padding;

	btCollisionObjectFloatData *m_body0;
	btCollisionObjectFloatData *m_body1;
};



#ifdef BT_USE_DOUBLE_PRECISION
#define btPersistentManifoldData btPersistentManifoldDoubleData
#define btPersistentManifoldDataName "btPersistentManifoldDoubleData"
#else
#define btPersistentManifoldData btPersistentManifoldFloatData
#define btPersistentManifoldDataName "btPersistentManifoldFloatData"
#endif  

#endif  
