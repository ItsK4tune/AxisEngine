

#ifndef BT_COLLISION_CREATE_FUNC
#define BT_COLLISION_CREATE_FUNC

#include "LinearMath/btAlignedObjectArray.h"
class btCollisionAlgorithm;
class btCollisionObject;
struct btCollisionObjectWrapper;
struct btCollisionAlgorithmConstructionInfo;


struct btCollisionAlgorithmCreateFunc
{
	bool m_swapped;

	btCollisionAlgorithmCreateFunc()
		: m_swapped(false)
	{
	}
	virtual ~btCollisionAlgorithmCreateFunc(){};

	virtual btCollisionAlgorithm* CreateCollisionAlgorithm(btCollisionAlgorithmConstructionInfo&, const btCollisionObjectWrapper* body0Wrap, const btCollisionObjectWrapper* body1Wrap)
	{
		(void)body0Wrap;
		(void)body1Wrap;
		return 0;
	}
};
#endif  
