

#ifndef BT_TRIANGLE_CALLBACK_H
#define BT_TRIANGLE_CALLBACK_H

#include "LinearMath/btVector3.h"



class btTriangleCallback
{
public:
	virtual ~btTriangleCallback();
	virtual void processTriangle(btVector3* triangle, int partId, int triangleIndex) = 0;
};

class btInternalTriangleIndexCallback
{
public:
	virtual ~btInternalTriangleIndexCallback();
	virtual void internalProcessTriangleIndex(btVector3* triangle, int partId, int triangleIndex) = 0;
};

#endif  
