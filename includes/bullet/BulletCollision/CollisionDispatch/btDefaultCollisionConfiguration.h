

#ifndef BT_DEFAULT_COLLISION_CONFIGURATION
#define BT_DEFAULT_COLLISION_CONFIGURATION

#include "btCollisionConfiguration.h"
class btVoronoiSimplexSolver;
class btConvexPenetrationDepthSolver;

struct btDefaultCollisionConstructionInfo
{
	btPoolAllocator* m_persistentManifoldPool;
	btPoolAllocator* m_collisionAlgorithmPool;
	int m_defaultMaxPersistentManifoldPoolSize;
	int m_defaultMaxCollisionAlgorithmPoolSize;
	int m_customCollisionAlgorithmMaxElementSize;
	int m_useEpaPenetrationAlgorithm;

	btDefaultCollisionConstructionInfo()
		: m_persistentManifoldPool(0),
		  m_collisionAlgorithmPool(0),
		  m_defaultMaxPersistentManifoldPoolSize(4096),
		  m_defaultMaxCollisionAlgorithmPoolSize(4096),
		  m_customCollisionAlgorithmMaxElementSize(0),
		  m_useEpaPenetrationAlgorithm(true)
	{
	}
};




class btDefaultCollisionConfiguration : public btCollisionConfiguration
{
protected:
	int m_persistentManifoldPoolSize;

	btPoolAllocator* m_persistentManifoldPool;
	bool m_ownsPersistentManifoldPool;

	btPoolAllocator* m_collisionAlgorithmPool;
	bool m_ownsCollisionAlgorithmPool;

	
	btConvexPenetrationDepthSolver* m_pdSolver;

	
	btCollisionAlgorithmCreateFunc* m_convexConvexCreateFunc;
	btCollisionAlgorithmCreateFunc* m_convexConcaveCreateFunc;
	btCollisionAlgorithmCreateFunc* m_swappedConvexConcaveCreateFunc;
	btCollisionAlgorithmCreateFunc* m_compoundCreateFunc;
	btCollisionAlgorithmCreateFunc* m_compoundCompoundCreateFunc;

	btCollisionAlgorithmCreateFunc* m_swappedCompoundCreateFunc;
	btCollisionAlgorithmCreateFunc* m_emptyCreateFunc;
	btCollisionAlgorithmCreateFunc* m_sphereSphereCF;
	btCollisionAlgorithmCreateFunc* m_sphereBoxCF;
	btCollisionAlgorithmCreateFunc* m_boxSphereCF;

	btCollisionAlgorithmCreateFunc* m_boxBoxCF;
	btCollisionAlgorithmCreateFunc* m_sphereTriangleCF;
	btCollisionAlgorithmCreateFunc* m_triangleSphereCF;
	btCollisionAlgorithmCreateFunc* m_planeConvexCF;
	btCollisionAlgorithmCreateFunc* m_convexPlaneCF;

public:
	btDefaultCollisionConfiguration(const btDefaultCollisionConstructionInfo& constructionInfo = btDefaultCollisionConstructionInfo());

	virtual ~btDefaultCollisionConfiguration();

	
	virtual btPoolAllocator* getPersistentManifoldPool()
	{
		return m_persistentManifoldPool;
	}

	virtual btPoolAllocator* getCollisionAlgorithmPool()
	{
		return m_collisionAlgorithmPool;
	}

	virtual btCollisionAlgorithmCreateFunc* getCollisionAlgorithmCreateFunc(int proxyType0, int proxyType1);

	virtual btCollisionAlgorithmCreateFunc* getClosestPointsAlgorithmCreateFunc(int proxyType0, int proxyType1);

	
	
	
	
	
	
	
	void setConvexConvexMultipointIterations(int numPerturbationIterations = 3, int minimumPointsPerturbationThreshold = 3);

	void setPlaneConvexMultipointIterations(int numPerturbationIterations = 3, int minimumPointsPerturbationThreshold = 3);
};

#endif  
