

#include "btConvexConcaveCollisionAlgorithm.h"
#include "LinearMath/btQuickprof.h"
#include "BulletCollision/CollisionDispatch/btCollisionObject.h"
#include "BulletCollision/CollisionShapes/btMultiSphereShape.h"
#include "BulletCollision/BroadphaseCollision/btBroadphaseProxy.h"
#include "BulletCollision/CollisionShapes/btConcaveShape.h"
#include "BulletCollision/CollisionDispatch/btManifoldResult.h"
#include "BulletCollision/NarrowPhaseCollision/btRaycastCallback.h"
#include "BulletCollision/CollisionShapes/btTriangleShape.h"
#include "BulletCollision/CollisionShapes/btSphereShape.h"
#include "LinearMath/btIDebugDraw.h"
#include "BulletCollision/NarrowPhaseCollision/btSubSimplexConvexCast.h"
#include "BulletCollision/CollisionDispatch/btCollisionObjectWrapper.h"
#include "BulletCollision/CollisionShapes/btSdfCollisionShape.h"

btConvexConcaveCollisionAlgorithm::btConvexConcaveCollisionAlgorithm(const btCollisionAlgorithmConstructionInfo& ci, const btCollisionObjectWrapper* body0Wrap, const btCollisionObjectWrapper* body1Wrap, bool isSwapped)
	: btActivatingCollisionAlgorithm(ci, body0Wrap, body1Wrap),
	  m_btConvexTriangleCallback(ci.m_dispatcher1, body0Wrap, body1Wrap, isSwapped),
	  m_isSwapped(isSwapped)
{
}

btConvexConcaveCollisionAlgorithm::~btConvexConcaveCollisionAlgorithm()
{
}

void btConvexConcaveCollisionAlgorithm::getAllContactManifolds(btManifoldArray& manifoldArray)
{
	if (m_btConvexTriangleCallback.m_manifoldPtr)
	{
		manifoldArray.push_back(m_btConvexTriangleCallback.m_manifoldPtr);
	}
}

btConvexTriangleCallback::btConvexTriangleCallback(btDispatcher* dispatcher, const btCollisionObjectWrapper* body0Wrap, const btCollisionObjectWrapper* body1Wrap, bool isSwapped) : m_dispatcher(dispatcher),
																																													 m_dispatchInfoPtr(0)
{
	m_convexBodyWrap = isSwapped ? body1Wrap : body0Wrap;
	m_triBodyWrap = isSwapped ? body0Wrap : body1Wrap;

	
	
	
	m_manifoldPtr = m_dispatcher->getNewManifold(m_convexBodyWrap->getCollisionObject(), m_triBodyWrap->getCollisionObject());

	clearCache();
}

btConvexTriangleCallback::~btConvexTriangleCallback()
{
	clearCache();
	m_dispatcher->releaseManifold(m_manifoldPtr);
}

void btConvexTriangleCallback::clearCache()
{
	m_dispatcher->clearManifold(m_manifoldPtr);
}

void btConvexTriangleCallback::processTriangle(btVector3* triangle, int partId, int triangleIndex)
{
	BT_PROFILE("btConvexTriangleCallback::processTriangle");

	if (!TestTriangleAgainstAabb2(triangle, m_aabbMin, m_aabbMax))
	{
		return;
	}

	
	

	btCollisionAlgorithmConstructionInfo ci;
	ci.m_dispatcher1 = m_dispatcher;

#if 0	
	
	
	if (m_dispatchInfoPtr && m_dispatchInfoPtr->m_debugDraw && (m_dispatchInfoPtr->m_debugDraw->getDebugMode() &btIDebugDraw::DBG_DrawWireframe ))
	{
		const btCollisionObject* ob = const_cast<btCollisionObject*>(m_triBodyWrap->getCollisionObject());
		btVector3 color(1,1,0);
		btTransform& tr = ob->getWorldTransform();
		m_dispatchInfoPtr->m_debugDraw->drawLine(tr(triangle[0]),tr(triangle[1]),color);
		m_dispatchInfoPtr->m_debugDraw->drawLine(tr(triangle[1]),tr(triangle[2]),color);
		m_dispatchInfoPtr->m_debugDraw->drawLine(tr(triangle[2]),tr(triangle[0]),color);
	}
#endif

	if (m_convexBodyWrap->getCollisionShape()->isConvex())
	{
#ifdef BT_ENABLE_CONVEX_CONCAVE_EARLY_OUT
    
		
		
		{
			const btVector3 v0 = m_triBodyWrap->getWorldTransform()*triangle[0];
			const btVector3 v1 = m_triBodyWrap->getWorldTransform()*triangle[1];
			const btVector3 v2 = m_triBodyWrap->getWorldTransform()*triangle[2];

			btVector3 triangle_normal_world = ( v1 - v0).cross(v2 - v0);
			triangle_normal_world.normalize();

		    btConvexShape* convex = (btConvexShape*)m_convexBodyWrap->getCollisionShape();
			
			btVector3 localPt = convex->localGetSupportingVertex(m_convexBodyWrap->getWorldTransform().getBasis().inverse()*triangle_normal_world);
			btVector3 worldPt = m_convexBodyWrap->getWorldTransform()*localPt;
			
			btScalar proj_distPt = triangle_normal_world.dot(worldPt);
			btScalar proj_distTr = triangle_normal_world.dot(v0);
			btScalar contact_threshold = m_manifoldPtr->getContactBreakingThreshold()+ m_resultOut->m_closestPointDistanceThreshold;
			btScalar dist = proj_distTr - proj_distPt;
			if (dist > contact_threshold)
				return;

			
			triangle_normal_world*=-1;

			localPt = convex->localGetSupportingVertex(m_convexBodyWrap->getWorldTransform().getBasis().inverse()*triangle_normal_world);
			worldPt = m_convexBodyWrap->getWorldTransform()*localPt;
			
			proj_distPt = triangle_normal_world.dot(worldPt);
			proj_distTr = triangle_normal_world.dot(v0);
			
			dist = proj_distTr - proj_distPt;
			if (dist > contact_threshold)
				return;
        }
#endif 

		btTriangleShape tm(triangle[0], triangle[1], triangle[2]);
		tm.setMargin(m_collisionMarginTriangle);

		btCollisionObjectWrapper triObWrap(m_triBodyWrap, &tm, m_triBodyWrap->getCollisionObject(), m_triBodyWrap->getWorldTransform(), partId, triangleIndex);  
		btCollisionAlgorithm* colAlgo = 0;

		if (m_resultOut->m_closestPointDistanceThreshold > 0)
		{
			colAlgo = ci.m_dispatcher1->findAlgorithm(m_convexBodyWrap, &triObWrap, 0, BT_CLOSEST_POINT_ALGORITHMS);
		}
		else
		{
			colAlgo = ci.m_dispatcher1->findAlgorithm(m_convexBodyWrap, &triObWrap, m_manifoldPtr, BT_CONTACT_POINT_ALGORITHMS);
		}
		const btCollisionObjectWrapper* tmpWrap = 0;

		if (m_resultOut->getBody0Internal() == m_triBodyWrap->getCollisionObject())
		{
			tmpWrap = m_resultOut->getBody0Wrap();
			m_resultOut->setBody0Wrap(&triObWrap);
			m_resultOut->setShapeIdentifiersA(partId, triangleIndex);
		}
		else
		{
			tmpWrap = m_resultOut->getBody1Wrap();
			m_resultOut->setBody1Wrap(&triObWrap);
			m_resultOut->setShapeIdentifiersB(partId, triangleIndex);
		}

		{
			BT_PROFILE("processCollision (GJK?)");
			colAlgo->processCollision(m_convexBodyWrap, &triObWrap, *m_dispatchInfoPtr, m_resultOut);
		}

		if (m_resultOut->getBody0Internal() == m_triBodyWrap->getCollisionObject())
		{
			m_resultOut->setBody0Wrap(tmpWrap);
		}
		else
		{
			m_resultOut->setBody1Wrap(tmpWrap);
		}

		colAlgo->~btCollisionAlgorithm();
		ci.m_dispatcher1->freeCollisionAlgorithm(colAlgo);
	}
}

void btConvexTriangleCallback::setTimeStepAndCounters(btScalar collisionMarginTriangle, const btDispatcherInfo& dispatchInfo, const btCollisionObjectWrapper* convexBodyWrap, const btCollisionObjectWrapper* triBodyWrap, btManifoldResult* resultOut)
{
	m_convexBodyWrap = convexBodyWrap;
	m_triBodyWrap = triBodyWrap;

	m_dispatchInfoPtr = &dispatchInfo;
	m_collisionMarginTriangle = collisionMarginTriangle;
	m_resultOut = resultOut;

	
	btTransform convexInTriangleSpace;
	convexInTriangleSpace = m_triBodyWrap->getWorldTransform().inverse() * m_convexBodyWrap->getWorldTransform();
	const btCollisionShape* convexShape = static_cast<const btCollisionShape*>(m_convexBodyWrap->getCollisionShape());
	
	convexShape->getAabb(convexInTriangleSpace, m_aabbMin, m_aabbMax);
	btScalar extraMargin = collisionMarginTriangle + resultOut->m_closestPointDistanceThreshold;

	btVector3 extra(extraMargin, extraMargin, extraMargin);

	m_aabbMax += extra;
	m_aabbMin -= extra;
}

void btConvexConcaveCollisionAlgorithm::clearCache()
{
	m_btConvexTriangleCallback.clearCache();
}

void btConvexConcaveCollisionAlgorithm::processCollision(const btCollisionObjectWrapper* body0Wrap, const btCollisionObjectWrapper* body1Wrap, const btDispatcherInfo& dispatchInfo, btManifoldResult* resultOut)
{
	BT_PROFILE("btConvexConcaveCollisionAlgorithm::processCollision");

	const btCollisionObjectWrapper* convexBodyWrap = m_isSwapped ? body1Wrap : body0Wrap;
	const btCollisionObjectWrapper* triBodyWrap = m_isSwapped ? body0Wrap : body1Wrap;

	if (triBodyWrap->getCollisionShape()->isConcave())
	{
		if (triBodyWrap->getCollisionShape()->getShapeType() == SDF_SHAPE_PROXYTYPE)
		{
			btSdfCollisionShape* sdfShape = (btSdfCollisionShape*)triBodyWrap->getCollisionShape();
			if (convexBodyWrap->getCollisionShape()->isConvex())
			{
				btConvexShape* convex = (btConvexShape*)convexBodyWrap->getCollisionShape();
				btAlignedObjectArray<btVector3> queryVertices;

				if (convex->isPolyhedral())
				{
					btPolyhedralConvexShape* poly = (btPolyhedralConvexShape*)convex;
					for (int v = 0; v < poly->getNumVertices(); v++)
					{
						btVector3 vtx;
						poly->getVertex(v, vtx);
						queryVertices.push_back(vtx);
					}
				}
				btScalar maxDist = SIMD_EPSILON;

				if (convex->getShapeType() == SPHERE_SHAPE_PROXYTYPE)
				{
					queryVertices.push_back(btVector3(0, 0, 0));
					btSphereShape* sphere = (btSphereShape*)convex;
					maxDist = sphere->getRadius() + SIMD_EPSILON;
				}
				if (queryVertices.size())
				{
					resultOut->setPersistentManifold(m_btConvexTriangleCallback.m_manifoldPtr);
					

					btPolyhedralConvexShape* poly = (btPolyhedralConvexShape*)convex;
					for (int v = 0; v < queryVertices.size(); v++)
					{
						const btVector3& vtx = queryVertices[v];
						btVector3 vtxWorldSpace = convexBodyWrap->getWorldTransform() * vtx;
						btVector3 vtxInSdf = triBodyWrap->getWorldTransform().invXform(vtxWorldSpace);

						btVector3 normalLocal;
						btScalar dist;
						if (sdfShape->queryPoint(vtxInSdf, dist, normalLocal))
						{
							if (dist <= maxDist)
							{
								normalLocal.safeNormalize();
								btVector3 normal = triBodyWrap->getWorldTransform().getBasis() * normalLocal;

								if (convex->getShapeType() == SPHERE_SHAPE_PROXYTYPE)
								{
									btSphereShape* sphere = (btSphereShape*)convex;
									dist -= sphere->getRadius();
									vtxWorldSpace -= sphere->getRadius() * normal;
								}
								resultOut->addContactPoint(normal, vtxWorldSpace - normal * dist, dist);
							}
						}
					}
					resultOut->refreshContactPoints();
				}
			}
		}
		else
		{
			const btConcaveShape* concaveShape = static_cast<const btConcaveShape*>(triBodyWrap->getCollisionShape());

			if (convexBodyWrap->getCollisionShape()->isConvex())
			{
				btScalar collisionMarginTriangle = concaveShape->getMargin();

				resultOut->setPersistentManifold(m_btConvexTriangleCallback.m_manifoldPtr);
				m_btConvexTriangleCallback.setTimeStepAndCounters(collisionMarginTriangle, dispatchInfo, convexBodyWrap, triBodyWrap, resultOut);

				m_btConvexTriangleCallback.m_manifoldPtr->setBodies(convexBodyWrap->getCollisionObject(), triBodyWrap->getCollisionObject());

				concaveShape->processAllTriangles(&m_btConvexTriangleCallback, m_btConvexTriangleCallback.getAabbMin(), m_btConvexTriangleCallback.getAabbMax());

				resultOut->refreshContactPoints();

				m_btConvexTriangleCallback.clearWrapperData();
			}
		}
	}
}

btScalar btConvexConcaveCollisionAlgorithm::calculateTimeOfImpact(btCollisionObject* body0, btCollisionObject* body1, const btDispatcherInfo& dispatchInfo, btManifoldResult* resultOut)
{
	(void)resultOut;
	(void)dispatchInfo;
	btCollisionObject* convexbody = m_isSwapped ? body1 : body0;
	btCollisionObject* triBody = m_isSwapped ? body0 : body1;

	

	
	
	btScalar squareMot0 = (convexbody->getInterpolationWorldTransform().getOrigin() - convexbody->getWorldTransform().getOrigin()).length2();
	if (squareMot0 < convexbody->getCcdSquareMotionThreshold())
	{
		return btScalar(1.);
	}

	
	
	

	btTransform triInv = triBody->getWorldTransform().inverse();
	btTransform convexFromLocal = triInv * convexbody->getWorldTransform();
	btTransform convexToLocal = triInv * convexbody->getInterpolationWorldTransform();

	struct LocalTriangleSphereCastCallback : public btTriangleCallback
	{
		btTransform m_ccdSphereFromTrans;
		btTransform m_ccdSphereToTrans;
		btTransform m_meshTransform;

		btScalar m_ccdSphereRadius;
		btScalar m_hitFraction;

		LocalTriangleSphereCastCallback(const btTransform& from, const btTransform& to, btScalar ccdSphereRadius, btScalar hitFraction)
			: m_ccdSphereFromTrans(from),
			  m_ccdSphereToTrans(to),
			  m_ccdSphereRadius(ccdSphereRadius),
			  m_hitFraction(hitFraction)
		{
		}

		virtual void processTriangle(btVector3* triangle, int partId, int triangleIndex)
		{
			BT_PROFILE("processTriangle");
			(void)partId;
			(void)triangleIndex;
			
			btTransform ident;
			ident.setIdentity();
			btConvexCast::CastResult castResult;
			castResult.m_fraction = m_hitFraction;
			btSphereShape pointShape(m_ccdSphereRadius);
			btTriangleShape triShape(triangle[0], triangle[1], triangle[2]);
			btVoronoiSimplexSolver simplexSolver;
			btSubsimplexConvexCast convexCaster(&pointShape, &triShape, &simplexSolver);
			
			
			

			if (convexCaster.calcTimeOfImpact(m_ccdSphereFromTrans, m_ccdSphereToTrans,
											  ident, ident, castResult))
			{
				if (m_hitFraction > castResult.m_fraction)
					m_hitFraction = castResult.m_fraction;
			}
		}
	};

	if (triBody->getCollisionShape()->isConcave())
	{
		btVector3 rayAabbMin = convexFromLocal.getOrigin();
		rayAabbMin.setMin(convexToLocal.getOrigin());
		btVector3 rayAabbMax = convexFromLocal.getOrigin();
		rayAabbMax.setMax(convexToLocal.getOrigin());
		btScalar ccdRadius0 = convexbody->getCcdSweptSphereRadius();
		rayAabbMin -= btVector3(ccdRadius0, ccdRadius0, ccdRadius0);
		rayAabbMax += btVector3(ccdRadius0, ccdRadius0, ccdRadius0);

		btScalar curHitFraction = btScalar(1.);  
		LocalTriangleSphereCastCallback raycastCallback(convexFromLocal, convexToLocal,
														convexbody->getCcdSweptSphereRadius(), curHitFraction);

		raycastCallback.m_hitFraction = convexbody->getHitFraction();

		btCollisionObject* concavebody = triBody;

		btConcaveShape* triangleMesh = (btConcaveShape*)concavebody->getCollisionShape();

		if (triangleMesh)
		{
			triangleMesh->processAllTriangles(&raycastCallback, rayAabbMin, rayAabbMax);
		}

		if (raycastCallback.m_hitFraction < convexbody->getHitFraction())
		{
			convexbody->setHitFraction(raycastCallback.m_hitFraction);
			return raycastCallback.m_hitFraction;
		}
	}

	return btScalar(1.);
}
