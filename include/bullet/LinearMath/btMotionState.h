

#ifndef BT_MOTIONSTATE_H
#define BT_MOTIONSTATE_H

#include "btTransform.h"



class btMotionState
{
public:
	virtual ~btMotionState()
	{
	}

	virtual void getWorldTransform(btTransform& worldTrans) const = 0;

	
	virtual void setWorldTransform(const btTransform& worldTrans) = 0;
};

#endif  
