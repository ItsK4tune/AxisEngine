

















#include "btAxisSweep3.h"

btAxisSweep3::btAxisSweep3(const btVector3& worldAabbMin, const btVector3& worldAabbMax, unsigned short int maxHandles, btOverlappingPairCache* pairCache, bool disableRaycastAccelerator)
	: btAxisSweep3Internal<unsigned short int>(worldAabbMin, worldAabbMax, 0xfffe, 0xffff, maxHandles, pairCache, disableRaycastAccelerator)
{
	
	btAssert(maxHandles > 1 && maxHandles < 32767);
}

bt32BitAxisSweep3::bt32BitAxisSweep3(const btVector3& worldAabbMin, const btVector3& worldAabbMax, unsigned int maxHandles, btOverlappingPairCache* pairCache, bool disableRaycastAccelerator)
	: btAxisSweep3Internal<unsigned int>(worldAabbMin, worldAabbMax, 0xfffffffe, 0x7fffffff, maxHandles, pairCache, disableRaycastAccelerator)
{
	
	btAssert(maxHandles > 1 && maxHandles < 2147483647);
}
