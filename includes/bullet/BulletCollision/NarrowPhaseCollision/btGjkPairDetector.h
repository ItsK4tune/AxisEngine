

#ifndef BT_GJK_PAIR_DETECTOR_H
#define BT_GJK_PAIR_DETECTOR_H

#include "btDiscreteCollisionDetectorInterface.h"
#include "BulletCollision/CollisionShapes/btCollisionMargin.h"

class btConvexShape;
#include "btSimplexSolverInterface.h"
class btConvexPenetrationDepthSolver;


class btGjkPairDetector : public btDiscreteCollisionDetectorInterface
{
	btVector3 m_cachedSeparatingAxis;
	btConvexPenetrationDepthSolver* m_penetrationDepthSolver;
	btSimplexSolverInterface* m_simplexSolver;
	const btConvexShape* m_minkowskiA;
	const btConvexShape* m_minkowskiB;
	int m_shapeTypeA;
	int m_shapeTypeB;
	btScalar m_marginA;
	btScalar m_marginB;

	bool m_ignoreMargin;
	btScalar m_cachedSeparatingDistance;

public:
	
	int m_lastUsedMethod;
	int m_curIter;
	int m_degenerateSimplex;
	int m_catchDegeneracies;
	int m_fixContactNormalDirection;

	btGjkPairDetector(const btConvexShape* objectA, const btConvexShape* objectB, btSimplexSolverInterface* simplexSolver, btConvexPenetrationDepthSolver* penetrationDepthSolver);
	btGjkPairDetector(const btConvexShape* objectA, const btConvexShape* objectB, int shapeTypeA, int shapeTypeB, btScalar marginA, btScalar marginB, btSimplexSolverInterface* simplexSolver, btConvexPenetrationDepthSolver* penetrationDepthSolver);
	virtual ~btGjkPairDetector(){};

	virtual void getClosestPoints(const ClosestPointInput& input, Result& output, class btIDebugDraw* debugDraw, bool swapResults = false);

	void getClosestPointsNonVirtual(const ClosestPointInput& input, Result& output, class btIDebugDraw* debugDraw);

	void setMinkowskiA(const btConvexShape* minkA)
	{
		m_minkowskiA = minkA;
	}

	void setMinkowskiB(const btConvexShape* minkB)
	{
		m_minkowskiB = minkB;
	}
	void setCachedSeparatingAxis(const btVector3& separatingAxis)
	{
		m_cachedSeparatingAxis = separatingAxis;
	}

	const btVector3& getCachedSeparatingAxis() const
	{
		return m_cachedSeparatingAxis;
	}
	btScalar getCachedSeparatingDistance() const
	{
		return m_cachedSeparatingDistance;
	}

	void setPenetrationDepthSolver(btConvexPenetrationDepthSolver* penetrationDepthSolver)
	{
		m_penetrationDepthSolver = penetrationDepthSolver;
	}

	
	void setIgnoreMargin(bool ignoreMargin)
	{
		m_ignoreMargin = ignoreMargin;
	}
};

#endif  
