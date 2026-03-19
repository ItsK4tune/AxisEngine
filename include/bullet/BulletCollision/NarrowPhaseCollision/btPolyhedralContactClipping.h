



#ifndef BT_POLYHEDRAL_CONTACT_CLIPPING_H
#define BT_POLYHEDRAL_CONTACT_CLIPPING_H

#include "LinearMath/btAlignedObjectArray.h"
#include "LinearMath/btTransform.h"
#include "btDiscreteCollisionDetectorInterface.h"

class btConvexPolyhedron;

typedef btAlignedObjectArray<btVector3> btVertexArray;


struct btPolyhedralContactClipping
{
	static void clipHullAgainstHull(const btVector3& separatingNormal1, const btConvexPolyhedron& hullA, const btConvexPolyhedron& hullB, const btTransform& transA, const btTransform& transB, const btScalar minDist, btScalar maxDist, btVertexArray& worldVertsB1, btVertexArray& worldVertsB2, btDiscreteCollisionDetectorInterface::Result& resultOut);

	static void clipFaceAgainstHull(const btVector3& separatingNormal, const btConvexPolyhedron& hullA, const btTransform& transA, btVertexArray& worldVertsB1, btVertexArray& worldVertsB2, const btScalar minDist, btScalar maxDist, btDiscreteCollisionDetectorInterface::Result& resultOut);

	static bool findSeparatingAxis(const btConvexPolyhedron& hullA, const btConvexPolyhedron& hullB, const btTransform& transA, const btTransform& transB, btVector3& sep, btDiscreteCollisionDetectorInterface::Result& resultOut);

	
	static void clipFace(const btVertexArray& pVtxIn, btVertexArray& ppVtxOut, const btVector3& planeNormalWS, btScalar planeEqWS);
};

#endif  
