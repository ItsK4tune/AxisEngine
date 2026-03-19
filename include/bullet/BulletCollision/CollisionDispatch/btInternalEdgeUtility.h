
#ifndef BT_INTERNAL_EDGE_UTILITY_H
#define BT_INTERNAL_EDGE_UTILITY_H

#include "LinearMath/btHashMap.h"
#include "LinearMath/btVector3.h"

#include "BulletCollision/CollisionShapes/btTriangleInfoMap.h"




class btBvhTriangleMeshShape;
class btCollisionObject;
struct btCollisionObjectWrapper;
class btManifoldPoint;
class btIDebugDraw;
class btHeightfieldTerrainShape;

enum btInternalEdgeAdjustFlags
{
	BT_TRIANGLE_CONVEX_BACKFACE_MODE = 1,
	BT_TRIANGLE_CONCAVE_DOUBLE_SIDED = 2,  
	BT_TRIANGLE_CONVEX_DOUBLE_SIDED = 4
};


void btGenerateInternalEdgeInfo(btBvhTriangleMeshShape* trimeshShape, btTriangleInfoMap* triangleInfoMap);

void btGenerateInternalEdgeInfo(btHeightfieldTerrainShape* trimeshShape, btTriangleInfoMap* triangleInfoMap);



void btAdjustInternalEdgeContacts(btManifoldPoint& cp, const btCollisionObjectWrapper* trimeshColObj0Wrap, const btCollisionObjectWrapper* otherColObj1Wrap, int partId0, int index0, int normalAdjustFlags = 0);





#ifdef BT_INTERNAL_EDGE_DEBUG_DRAW
void btSetDebugDrawer(btIDebugDraw* debugDrawer);
#endif  

#endif  
