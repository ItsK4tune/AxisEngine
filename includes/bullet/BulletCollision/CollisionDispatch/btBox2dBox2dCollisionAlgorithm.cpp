




#include "btBox2dBox2dCollisionAlgorithm.h"
#include "BulletCollision/CollisionDispatch/btCollisionDispatcher.h"
#include "BulletCollision/CollisionShapes/btBoxShape.h"
#include "BulletCollision/CollisionDispatch/btCollisionObject.h"
#include "BulletCollision/CollisionDispatch/btBoxBoxDetector.h"
#include "BulletCollision/CollisionShapes/btBox2dShape.h"
#include "BulletCollision/CollisionDispatch/btCollisionObjectWrapper.h"

#define USE_PERSISTENT_CONTACTS 1

btBox2dBox2dCollisionAlgorithm::btBox2dBox2dCollisionAlgorithm(btPersistentManifold* mf, const btCollisionAlgorithmConstructionInfo& ci, const btCollisionObjectWrapper* obj0Wrap, const btCollisionObjectWrapper* obj1Wrap)
	: btActivatingCollisionAlgorithm(ci, obj0Wrap, obj1Wrap),
	  m_ownManifold(false),
	  m_manifoldPtr(mf)
{
	if (!m_manifoldPtr && m_dispatcher->needsCollision(obj0Wrap->getCollisionObject(), obj1Wrap->getCollisionObject()))
	{
		m_manifoldPtr = m_dispatcher->getNewManifold(obj0Wrap->getCollisionObject(), obj1Wrap->getCollisionObject());
		m_ownManifold = true;
	}
}

btBox2dBox2dCollisionAlgorithm::~btBox2dBox2dCollisionAlgorithm()
{
	if (m_ownManifold)
	{
		if (m_manifoldPtr)
			m_dispatcher->releaseManifold(m_manifoldPtr);
	}
}

void b2CollidePolygons(btManifoldResult* manifold, const btBox2dShape* polyA, const btTransform& xfA, const btBox2dShape* polyB, const btTransform& xfB);


void btBox2dBox2dCollisionAlgorithm::processCollision(const btCollisionObjectWrapper* body0Wrap, const btCollisionObjectWrapper* body1Wrap, const btDispatcherInfo& dispatchInfo, btManifoldResult* resultOut)
{
	if (!m_manifoldPtr)
		return;

	const btBox2dShape* box0 = (const btBox2dShape*)body0Wrap->getCollisionShape();
	const btBox2dShape* box1 = (const btBox2dShape*)body1Wrap->getCollisionShape();

	resultOut->setPersistentManifold(m_manifoldPtr);

	b2CollidePolygons(resultOut, box0, body0Wrap->getWorldTransform(), box1, body1Wrap->getWorldTransform());

	
	if (m_ownManifold)
	{
		resultOut->refreshContactPoints();
	}
}

btScalar btBox2dBox2dCollisionAlgorithm::calculateTimeOfImpact(btCollisionObject* , btCollisionObject* , const btDispatcherInfo& , btManifoldResult* )
{
	
	return 1.f;
}

struct ClipVertex
{
	btVector3 v;
	int id;
	
	
};

#define b2Dot(a, b) (a).dot(b)
#define b2Mul(a, b) (a) * (b)
#define b2MulT(a, b) (a).transpose() * (b)
#define b2Cross(a, b) (a).cross(b)
#define btCrossS(a, s) btVector3(s* a.getY(), -s* a.getX(), 0.f)

int b2_maxManifoldPoints = 2;

static int ClipSegmentToLine(ClipVertex vOut[2], ClipVertex vIn[2],
							 const btVector3& normal, btScalar offset)
{
	
	int numOut = 0;

	
	btScalar distance0 = b2Dot(normal, vIn[0].v) - offset;
	btScalar distance1 = b2Dot(normal, vIn[1].v) - offset;

	
	if (distance0 <= 0.0f) vOut[numOut++] = vIn[0];
	if (distance1 <= 0.0f) vOut[numOut++] = vIn[1];

	
	if (distance0 * distance1 < 0.0f)
	{
		
		btScalar interp = distance0 / (distance0 - distance1);
		vOut[numOut].v = vIn[0].v + interp * (vIn[1].v - vIn[0].v);
		if (distance0 > 0.0f)
		{
			vOut[numOut].id = vIn[0].id;
		}
		else
		{
			vOut[numOut].id = vIn[1].id;
		}
		++numOut;
	}

	return numOut;
}


static btScalar EdgeSeparation(const btBox2dShape* poly1, const btTransform& xf1, int edge1,
							   const btBox2dShape* poly2, const btTransform& xf2)
{
	const btVector3* vertices1 = poly1->getVertices();
	const btVector3* normals1 = poly1->getNormals();

	int count2 = poly2->getVertexCount();
	const btVector3* vertices2 = poly2->getVertices();

	btAssert(0 <= edge1 && edge1 < poly1->getVertexCount());

	
	btVector3 normal1World = b2Mul(xf1.getBasis(), normals1[edge1]);
	btVector3 normal1 = b2MulT(xf2.getBasis(), normal1World);

	
	int index = 0;
	btScalar minDot = BT_LARGE_FLOAT;

	if (count2 > 0)
		index = (int)normal1.minDot(vertices2, count2, minDot);

	btVector3 v1 = b2Mul(xf1, vertices1[edge1]);
	btVector3 v2 = b2Mul(xf2, vertices2[index]);
	btScalar separation = b2Dot(v2 - v1, normal1World);
	return separation;
}


static btScalar FindMaxSeparation(int* edgeIndex,
								  const btBox2dShape* poly1, const btTransform& xf1,
								  const btBox2dShape* poly2, const btTransform& xf2)
{
	int count1 = poly1->getVertexCount();
	const btVector3* normals1 = poly1->getNormals();

	
	btVector3 d = b2Mul(xf2, poly2->getCentroid()) - b2Mul(xf1, poly1->getCentroid());
	btVector3 dLocal1 = b2MulT(xf1.getBasis(), d);

	
	int edge = 0;
	btScalar maxDot;
	if (count1 > 0)
		edge = (int)dLocal1.maxDot(normals1, count1, maxDot);

	
	btScalar s = EdgeSeparation(poly1, xf1, edge, poly2, xf2);
	if (s > 0.0f)
	{
		return s;
	}

	
	int prevEdge = edge - 1 >= 0 ? edge - 1 : count1 - 1;
	btScalar sPrev = EdgeSeparation(poly1, xf1, prevEdge, poly2, xf2);
	if (sPrev > 0.0f)
	{
		return sPrev;
	}

	
	int nextEdge = edge + 1 < count1 ? edge + 1 : 0;
	btScalar sNext = EdgeSeparation(poly1, xf1, nextEdge, poly2, xf2);
	if (sNext > 0.0f)
	{
		return sNext;
	}

	
	int bestEdge;
	btScalar bestSeparation;
	int increment;
	if (sPrev > s && sPrev > sNext)
	{
		increment = -1;
		bestEdge = prevEdge;
		bestSeparation = sPrev;
	}
	else if (sNext > s)
	{
		increment = 1;
		bestEdge = nextEdge;
		bestSeparation = sNext;
	}
	else
	{
		*edgeIndex = edge;
		return s;
	}

	
	for (;;)
	{
		if (increment == -1)
			edge = bestEdge - 1 >= 0 ? bestEdge - 1 : count1 - 1;
		else
			edge = bestEdge + 1 < count1 ? bestEdge + 1 : 0;

		s = EdgeSeparation(poly1, xf1, edge, poly2, xf2);
		if (s > 0.0f)
		{
			return s;
		}

		if (s > bestSeparation)
		{
			bestEdge = edge;
			bestSeparation = s;
		}
		else
		{
			break;
		}
	}

	*edgeIndex = bestEdge;
	return bestSeparation;
}

static void FindIncidentEdge(ClipVertex c[2],
							 const btBox2dShape* poly1, const btTransform& xf1, int edge1,
							 const btBox2dShape* poly2, const btTransform& xf2)
{
	const btVector3* normals1 = poly1->getNormals();

	int count2 = poly2->getVertexCount();
	const btVector3* vertices2 = poly2->getVertices();
	const btVector3* normals2 = poly2->getNormals();

	btAssert(0 <= edge1 && edge1 < poly1->getVertexCount());

	
	btVector3 normal1 = b2MulT(xf2.getBasis(), b2Mul(xf1.getBasis(), normals1[edge1]));

	
	int index = 0;
	btScalar minDot = BT_LARGE_FLOAT;
	for (int i = 0; i < count2; ++i)
	{
		btScalar dot = b2Dot(normal1, normals2[i]);
		if (dot < minDot)
		{
			minDot = dot;
			index = i;
		}
	}

	
	int i1 = index;
	int i2 = i1 + 1 < count2 ? i1 + 1 : 0;

	c[0].v = b2Mul(xf2, vertices2[i1]);
	
	
	

	c[1].v = b2Mul(xf2, vertices2[i2]);
	
	
	
}








void b2CollidePolygons(btManifoldResult* manifold,
					   const btBox2dShape* polyA, const btTransform& xfA,
					   const btBox2dShape* polyB, const btTransform& xfB)
{
	int edgeA = 0;
	btScalar separationA = FindMaxSeparation(&edgeA, polyA, xfA, polyB, xfB);
	if (separationA > 0.0f)
		return;

	int edgeB = 0;
	btScalar separationB = FindMaxSeparation(&edgeB, polyB, xfB, polyA, xfA);
	if (separationB > 0.0f)
		return;

	const btBox2dShape* poly1;  
	const btBox2dShape* poly2;  
	btTransform xf1, xf2;
	int edge1;  
	unsigned char flip;
	const btScalar k_relativeTol = 0.98f;
	const btScalar k_absoluteTol = 0.001f;

	
	if (separationB > k_relativeTol * separationA + k_absoluteTol)
	{
		poly1 = polyB;
		poly2 = polyA;
		xf1 = xfB;
		xf2 = xfA;
		edge1 = edgeB;
		flip = 1;
	}
	else
	{
		poly1 = polyA;
		poly2 = polyB;
		xf1 = xfA;
		xf2 = xfB;
		edge1 = edgeA;
		flip = 0;
	}

	ClipVertex incidentEdge[2];
	FindIncidentEdge(incidentEdge, poly1, xf1, edge1, poly2, xf2);

	int count1 = poly1->getVertexCount();
	const btVector3* vertices1 = poly1->getVertices();

	btVector3 v11 = vertices1[edge1];
	btVector3 v12 = edge1 + 1 < count1 ? vertices1[edge1 + 1] : vertices1[0];

	
	btVector3 sideNormal = b2Mul(xf1.getBasis(), v12 - v11);
	sideNormal.normalize();
	btVector3 frontNormal = btCrossS(sideNormal, 1.0f);

	v11 = b2Mul(xf1, v11);
	v12 = b2Mul(xf1, v12);

	btScalar frontOffset = b2Dot(frontNormal, v11);
	btScalar sideOffset1 = -b2Dot(sideNormal, v11);
	btScalar sideOffset2 = b2Dot(sideNormal, v12);

	
	ClipVertex clipPoints1[2];
	clipPoints1[0].v.setValue(0, 0, 0);
	clipPoints1[1].v.setValue(0, 0, 0);

	ClipVertex clipPoints2[2];
	clipPoints2[0].v.setValue(0, 0, 0);
	clipPoints2[1].v.setValue(0, 0, 0);

	int np;

	
	np = ClipSegmentToLine(clipPoints1, incidentEdge, -sideNormal, sideOffset1);

	if (np < 2)
		return;

	
	np = ClipSegmentToLine(clipPoints2, clipPoints1, sideNormal, sideOffset2);

	if (np < 2)
	{
		return;
	}

	
	btVector3 manifoldNormal = flip ? -frontNormal : frontNormal;

	int pointCount = 0;
	for (int i = 0; i < b2_maxManifoldPoints; ++i)
	{
		btScalar separation = b2Dot(frontNormal, clipPoints2[i].v) - frontOffset;

		if (separation <= 0.0f)
		{
			
			
			
			

			manifold->addContactPoint(-manifoldNormal, clipPoints2[i].v, separation);

			
			
			++pointCount;
		}
	}

	
}
