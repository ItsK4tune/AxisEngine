

#ifndef BT_BROADPHASE_INTERFACE_H
#define BT_BROADPHASE_INTERFACE_H

struct btDispatcherInfo;
class btDispatcher;
#include "btBroadphaseProxy.h"

class btOverlappingPairCache;

struct btBroadphaseAabbCallback
{
	virtual ~btBroadphaseAabbCallback() {}
	virtual bool process(const btBroadphaseProxy* proxy) = 0;
};

struct btBroadphaseRayCallback : public btBroadphaseAabbCallback
{
	
	btVector3 m_rayDirectionInverse;
	unsigned int m_signs[3];
	btScalar m_lambda_max;

	virtual ~btBroadphaseRayCallback() {}

protected:
	btBroadphaseRayCallback() {}
};

#include "LinearMath/btVector3.h"




class btBroadphaseInterface
{
public:
	virtual ~btBroadphaseInterface() {}

	virtual btBroadphaseProxy* createProxy(const btVector3& aabbMin, const btVector3& aabbMax, int shapeType, void* userPtr, int collisionFilterGroup, int collisionFilterMask, btDispatcher* dispatcher) = 0;
	virtual void destroyProxy(btBroadphaseProxy* proxy, btDispatcher* dispatcher) = 0;
	virtual void setAabb(btBroadphaseProxy* proxy, const btVector3& aabbMin, const btVector3& aabbMax, btDispatcher* dispatcher) = 0;
	virtual void getAabb(btBroadphaseProxy* proxy, btVector3& aabbMin, btVector3& aabbMax) const = 0;

	virtual void rayTest(const btVector3& rayFrom, const btVector3& rayTo, btBroadphaseRayCallback& rayCallback, const btVector3& aabbMin = btVector3(0, 0, 0), const btVector3& aabbMax = btVector3(0, 0, 0)) = 0;

	virtual void aabbTest(const btVector3& aabbMin, const btVector3& aabbMax, btBroadphaseAabbCallback& callback) = 0;

	
	virtual void calculateOverlappingPairs(btDispatcher* dispatcher) = 0;

	virtual btOverlappingPairCache* getOverlappingPairCache() = 0;
	virtual const btOverlappingPairCache* getOverlappingPairCache() const = 0;

	
	
	virtual void getBroadphaseAabb(btVector3& aabbMin, btVector3& aabbMax) const = 0;

	
	virtual void resetPool(btDispatcher* dispatcher) { (void)dispatcher; };

	virtual void printStats() = 0;
};

#endif  
