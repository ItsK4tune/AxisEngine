

















#ifndef BT_AXIS_SWEEP_3_H
#define BT_AXIS_SWEEP_3_H

#include "LinearMath/btVector3.h"
#include "btOverlappingPairCache.h"
#include "btBroadphaseInterface.h"
#include "btBroadphaseProxy.h"
#include "btOverlappingPairCallback.h"
#include "btDbvtBroadphase.h"
#include "btAxisSweep3Internal.h"




class btAxisSweep3 : public btAxisSweep3Internal<unsigned short int>
{
public:
	btAxisSweep3(const btVector3& worldAabbMin, const btVector3& worldAabbMax, unsigned short int maxHandles = 16384, btOverlappingPairCache* pairCache = 0, bool disableRaycastAccelerator = false);
};




class bt32BitAxisSweep3 : public btAxisSweep3Internal<unsigned int>
{
public:
	bt32BitAxisSweep3(const btVector3& worldAabbMin, const btVector3& worldAabbMax, unsigned int maxHandles = 1500000, btOverlappingPairCache* pairCache = 0, bool disableRaycastAccelerator = false);
};

#endif
