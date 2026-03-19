

#ifndef __BT_ACTIVATING_COLLISION_ALGORITHM_H
#define __BT_ACTIVATING_COLLISION_ALGORITHM_H

#include "BulletCollision/BroadphaseCollision/btCollisionAlgorithm.h"


class btActivatingCollisionAlgorithm : public btCollisionAlgorithm
{
	
	

protected:
	btActivatingCollisionAlgorithm(const btCollisionAlgorithmConstructionInfo& ci);

	btActivatingCollisionAlgorithm(const btCollisionAlgorithmConstructionInfo& ci, const btCollisionObjectWrapper* body0Wrap, const btCollisionObjectWrapper* body1Wrap);

public:
	virtual ~btActivatingCollisionAlgorithm();
};
#endif  
