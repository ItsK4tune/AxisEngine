

#ifndef BT_COLLISION_CONFIGURATION
#define BT_COLLISION_CONFIGURATION

struct btCollisionAlgorithmCreateFunc;

class btPoolAllocator;




class btCollisionConfiguration
{
public:
	virtual ~btCollisionConfiguration()
	{
	}

	
	virtual btPoolAllocator* getPersistentManifoldPool() = 0;

	virtual btPoolAllocator* getCollisionAlgorithmPool() = 0;

	virtual btCollisionAlgorithmCreateFunc* getCollisionAlgorithmCreateFunc(int proxyType0, int proxyType1) = 0;

	virtual btCollisionAlgorithmCreateFunc* getClosestPointsAlgorithmCreateFunc(int proxyType0, int proxyType1) = 0;
};

#endif  
