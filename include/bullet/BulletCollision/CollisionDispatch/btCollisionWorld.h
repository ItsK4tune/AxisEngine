



#ifndef BT_COLLISION_WORLD_H
#define BT_COLLISION_WORLD_H

class btCollisionShape;
class btConvexShape;
class btBroadphaseInterface;
class btSerializer;

#include "LinearMath/btVector3.h"
#include "LinearMath/btTransform.h"
#include "btCollisionObject.h"
#include "btCollisionDispatcher.h"
#include "BulletCollision/BroadphaseCollision/btOverlappingPairCache.h"
#include "LinearMath/btAlignedObjectArray.h"


class btCollisionWorld
{
protected:
	btAlignedObjectArray<btCollisionObject*> m_collisionObjects;

	btDispatcher* m_dispatcher1;

	btDispatcherInfo m_dispatchInfo;

	btBroadphaseInterface* m_broadphasePairCache;

	btIDebugDraw* m_debugDrawer;

	
	
	bool m_forceUpdateAllAabbs;

	void serializeCollisionObjects(btSerializer* serializer);

	void serializeContactManifolds(btSerializer* serializer);

public:
	
	btCollisionWorld(btDispatcher* dispatcher, btBroadphaseInterface* broadphasePairCache, btCollisionConfiguration* collisionConfiguration);

	virtual ~btCollisionWorld();

	void setBroadphase(btBroadphaseInterface* pairCache)
	{
		m_broadphasePairCache = pairCache;
	}

	const btBroadphaseInterface* getBroadphase() const
	{
		return m_broadphasePairCache;
	}

	btBroadphaseInterface* getBroadphase()
	{
		return m_broadphasePairCache;
	}

	btOverlappingPairCache* getPairCache()
	{
		return m_broadphasePairCache->getOverlappingPairCache();
	}

	btDispatcher* getDispatcher()
	{
		return m_dispatcher1;
	}

	const btDispatcher* getDispatcher() const
	{
		return m_dispatcher1;
	}

	void updateSingleAabb(btCollisionObject* colObj);

	virtual void updateAabbs();

	
	
	virtual void computeOverlappingPairs();

	virtual void setDebugDrawer(btIDebugDraw* debugDrawer)
	{
		m_debugDrawer = debugDrawer;
	}

	virtual btIDebugDraw* getDebugDrawer()
	{
		return m_debugDrawer;
	}

	virtual void debugDrawWorld();

	virtual void debugDrawObject(const btTransform& worldTransform, const btCollisionShape* shape, const btVector3& color);

	
	
	struct LocalShapeInfo
	{
		int m_shapePart;
		int m_triangleIndex;

		
		
	};

	struct LocalRayResult
	{
		LocalRayResult(const btCollisionObject* collisionObject,
					   LocalShapeInfo* localShapeInfo,
					   const btVector3& hitNormalLocal,
					   btScalar hitFraction)
			: m_collisionObject(collisionObject),
			  m_localShapeInfo(localShapeInfo),
			  m_hitNormalLocal(hitNormalLocal),
			  m_hitFraction(hitFraction)
		{
		}

		const btCollisionObject* m_collisionObject;
		LocalShapeInfo* m_localShapeInfo;
		btVector3 m_hitNormalLocal;
		btScalar m_hitFraction;
	};

	
	struct RayResultCallback
	{
		btScalar m_closestHitFraction;
		const btCollisionObject* m_collisionObject;
		int m_collisionFilterGroup;
		int m_collisionFilterMask;
		
		unsigned int m_flags;

		virtual ~RayResultCallback()
		{
		}
		bool hasHit() const
		{
			return (m_collisionObject != 0);
		}

		RayResultCallback()
			: m_closestHitFraction(btScalar(1.)),
			  m_collisionObject(0),
			  m_collisionFilterGroup(btBroadphaseProxy::DefaultFilter),
			  m_collisionFilterMask(btBroadphaseProxy::AllFilter),
			  
			  m_flags(0)
		{
		}

		virtual bool needsCollision(btBroadphaseProxy* proxy0) const
		{
			bool collides = (proxy0->m_collisionFilterGroup & m_collisionFilterMask) != 0;
			collides = collides && (m_collisionFilterGroup & proxy0->m_collisionFilterMask);
			return collides;
		}

		virtual btScalar addSingleResult(LocalRayResult& rayResult, bool normalInWorldSpace) = 0;
	};

	struct ClosestRayResultCallback : public RayResultCallback
	{
		ClosestRayResultCallback(const btVector3& rayFromWorld, const btVector3& rayToWorld)
			: m_rayFromWorld(rayFromWorld),
			  m_rayToWorld(rayToWorld)
		{
		}

		btVector3 m_rayFromWorld;  
		btVector3 m_rayToWorld;

		btVector3 m_hitNormalWorld;
		btVector3 m_hitPointWorld;

		virtual btScalar addSingleResult(LocalRayResult& rayResult, bool normalInWorldSpace)
		{
			
			btAssert(rayResult.m_hitFraction <= m_closestHitFraction);

			m_closestHitFraction = rayResult.m_hitFraction;
			m_collisionObject = rayResult.m_collisionObject;
			if (normalInWorldSpace)
			{
				m_hitNormalWorld = rayResult.m_hitNormalLocal;
			}
			else
			{
				
				m_hitNormalWorld = m_collisionObject->getWorldTransform().getBasis() * rayResult.m_hitNormalLocal;
			}
			m_hitPointWorld.setInterpolate3(m_rayFromWorld, m_rayToWorld, rayResult.m_hitFraction);
			return rayResult.m_hitFraction;
		}
	};

	struct AllHitsRayResultCallback : public RayResultCallback
	{
		AllHitsRayResultCallback(const btVector3& rayFromWorld, const btVector3& rayToWorld)
			: m_rayFromWorld(rayFromWorld),
			  m_rayToWorld(rayToWorld)
		{
		}

		btAlignedObjectArray<const btCollisionObject*> m_collisionObjects;

		btVector3 m_rayFromWorld;  
		btVector3 m_rayToWorld;

		btAlignedObjectArray<btVector3> m_hitNormalWorld;
		btAlignedObjectArray<btVector3> m_hitPointWorld;
		btAlignedObjectArray<btScalar> m_hitFractions;

		virtual btScalar addSingleResult(LocalRayResult& rayResult, bool normalInWorldSpace)
		{
			m_collisionObject = rayResult.m_collisionObject;
			m_collisionObjects.push_back(rayResult.m_collisionObject);
			btVector3 hitNormalWorld;
			if (normalInWorldSpace)
			{
				hitNormalWorld = rayResult.m_hitNormalLocal;
			}
			else
			{
				
				hitNormalWorld = m_collisionObject->getWorldTransform().getBasis() * rayResult.m_hitNormalLocal;
			}
			m_hitNormalWorld.push_back(hitNormalWorld);
			btVector3 hitPointWorld;
			hitPointWorld.setInterpolate3(m_rayFromWorld, m_rayToWorld, rayResult.m_hitFraction);
			m_hitPointWorld.push_back(hitPointWorld);
			m_hitFractions.push_back(rayResult.m_hitFraction);
			return m_closestHitFraction;
		}
	};

	struct LocalConvexResult
	{
		LocalConvexResult(const btCollisionObject* hitCollisionObject,
						  LocalShapeInfo* localShapeInfo,
						  const btVector3& hitNormalLocal,
						  const btVector3& hitPointLocal,
						  btScalar hitFraction)
			: m_hitCollisionObject(hitCollisionObject),
			  m_localShapeInfo(localShapeInfo),
			  m_hitNormalLocal(hitNormalLocal),
			  m_hitPointLocal(hitPointLocal),
			  m_hitFraction(hitFraction)
		{
		}

		const btCollisionObject* m_hitCollisionObject;
		LocalShapeInfo* m_localShapeInfo;
		btVector3 m_hitNormalLocal;
		btVector3 m_hitPointLocal;
		btScalar m_hitFraction;
	};

	
	struct ConvexResultCallback
	{
		btScalar m_closestHitFraction;
		int m_collisionFilterGroup;
		int m_collisionFilterMask;

		ConvexResultCallback()
			: m_closestHitFraction(btScalar(1.)),
			  m_collisionFilterGroup(btBroadphaseProxy::DefaultFilter),
			  m_collisionFilterMask(btBroadphaseProxy::AllFilter)
		{
		}

		virtual ~ConvexResultCallback()
		{
		}

		bool hasHit() const
		{
			return (m_closestHitFraction < btScalar(1.));
		}

		virtual bool needsCollision(btBroadphaseProxy* proxy0) const
		{
			bool collides = (proxy0->m_collisionFilterGroup & m_collisionFilterMask) != 0;
			collides = collides && (m_collisionFilterGroup & proxy0->m_collisionFilterMask);
			return collides;
		}

		virtual btScalar addSingleResult(LocalConvexResult& convexResult, bool normalInWorldSpace) = 0;
	};

	struct ClosestConvexResultCallback : public ConvexResultCallback
	{
		ClosestConvexResultCallback(const btVector3& convexFromWorld, const btVector3& convexToWorld)
			: m_convexFromWorld(convexFromWorld),
			  m_convexToWorld(convexToWorld),
			  m_hitCollisionObject(0)
		{
		}

		btVector3 m_convexFromWorld;  
		btVector3 m_convexToWorld;

		btVector3 m_hitNormalWorld;
		btVector3 m_hitPointWorld;
		const btCollisionObject* m_hitCollisionObject;

		virtual btScalar addSingleResult(LocalConvexResult& convexResult, bool normalInWorldSpace)
		{
			
			btAssert(convexResult.m_hitFraction <= m_closestHitFraction);

			m_closestHitFraction = convexResult.m_hitFraction;
			m_hitCollisionObject = convexResult.m_hitCollisionObject;
			if (normalInWorldSpace)
			{
				m_hitNormalWorld = convexResult.m_hitNormalLocal;
			}
			else
			{
				
				m_hitNormalWorld = m_hitCollisionObject->getWorldTransform().getBasis() * convexResult.m_hitNormalLocal;
			}
			m_hitPointWorld = convexResult.m_hitPointLocal;
			return convexResult.m_hitFraction;
		}
	};

	
	struct ContactResultCallback
	{
		int m_collisionFilterGroup;
		int m_collisionFilterMask;
		btScalar m_closestDistanceThreshold;

		ContactResultCallback()
			: m_collisionFilterGroup(btBroadphaseProxy::DefaultFilter),
			  m_collisionFilterMask(btBroadphaseProxy::AllFilter),
			  m_closestDistanceThreshold(0)
		{
		}

		virtual ~ContactResultCallback()
		{
		}

		virtual bool needsCollision(btBroadphaseProxy* proxy0) const
		{
			bool collides = (proxy0->m_collisionFilterGroup & m_collisionFilterMask) != 0;
			collides = collides && (m_collisionFilterGroup & proxy0->m_collisionFilterMask);
			return collides;
		}

		virtual btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1) = 0;
	};

	int getNumCollisionObjects() const
	{
		return int(m_collisionObjects.size());
	}

	
	
	virtual void rayTest(const btVector3& rayFromWorld, const btVector3& rayToWorld, RayResultCallback& resultCallback) const;

	
	
	void convexSweepTest(const btConvexShape* castShape, const btTransform& from, const btTransform& to, ConvexResultCallback& resultCallback, btScalar allowedCcdPenetration = btScalar(0.)) const;

	
	
	void contactTest(btCollisionObject* colObj, ContactResultCallback& resultCallback);

	
	
	void contactPairTest(btCollisionObject* colObjA, btCollisionObject* colObjB, ContactResultCallback& resultCallback);

	
	
	
	static void rayTestSingle(const btTransform& rayFromTrans, const btTransform& rayToTrans,
							  btCollisionObject* collisionObject,
							  const btCollisionShape* collisionShape,
							  const btTransform& colObjWorldTransform,
							  RayResultCallback& resultCallback);

	static void rayTestSingleInternal(const btTransform& rayFromTrans, const btTransform& rayToTrans,
									  const btCollisionObjectWrapper* collisionObjectWrap,
									  RayResultCallback& resultCallback);

	
	static void objectQuerySingle(const btConvexShape* castShape, const btTransform& rayFromTrans, const btTransform& rayToTrans,
								  btCollisionObject* collisionObject,
								  const btCollisionShape* collisionShape,
								  const btTransform& colObjWorldTransform,
								  ConvexResultCallback& resultCallback, btScalar allowedPenetration);

	static void objectQuerySingleInternal(const btConvexShape* castShape, const btTransform& convexFromTrans, const btTransform& convexToTrans,
										  const btCollisionObjectWrapper* colObjWrap,
										  ConvexResultCallback& resultCallback, btScalar allowedPenetration);

	virtual void addCollisionObject(btCollisionObject* collisionObject, int collisionFilterGroup = btBroadphaseProxy::DefaultFilter, int collisionFilterMask = btBroadphaseProxy::AllFilter);

	virtual void refreshBroadphaseProxy(btCollisionObject* collisionObject);

	btCollisionObjectArray& getCollisionObjectArray()
	{
		return m_collisionObjects;
	}

	const btCollisionObjectArray& getCollisionObjectArray() const
	{
		return m_collisionObjects;
	}

	virtual void removeCollisionObject(btCollisionObject* collisionObject);

	virtual void performDiscreteCollisionDetection();

	btDispatcherInfo& getDispatchInfo()
	{
		return m_dispatchInfo;
	}

	const btDispatcherInfo& getDispatchInfo() const
	{
		return m_dispatchInfo;
	}

	bool getForceUpdateAllAabbs() const
	{
		return m_forceUpdateAllAabbs;
	}
	void setForceUpdateAllAabbs(bool forceUpdateAllAabbs)
	{
		m_forceUpdateAllAabbs = forceUpdateAllAabbs;
	}

	
	virtual void serialize(btSerializer* serializer);
};

#endif  
