

#ifndef BT_CONVEX_CAST_H
#define BT_CONVEX_CAST_H

#include "LinearMath/btTransform.h"
#include "LinearMath/btVector3.h"
#include "LinearMath/btScalar.h"
class btMinkowskiSumShape;
#include "LinearMath/btIDebugDraw.h"

#ifdef BT_USE_DOUBLE_PRECISION
#define MAX_CONVEX_CAST_ITERATIONS 64
#define MAX_CONVEX_CAST_EPSILON (SIMD_EPSILON * 10)
#else
#define MAX_CONVEX_CAST_ITERATIONS 32
#define MAX_CONVEX_CAST_EPSILON btScalar(0.0001)
#endif







class btConvexCast
{
public:
	virtual ~btConvexCast();

	
	
	struct CastResult
	{
		

		virtual void DebugDraw(btScalar fraction) { (void)fraction; }
		virtual void drawCoordSystem(const btTransform& trans) { (void)trans; }
		virtual void reportFailure(int errNo, int numIterations)
		{
			(void)errNo;
			(void)numIterations;
		}
		CastResult()
			: m_fraction(btScalar(BT_LARGE_FLOAT)),
			  m_debugDrawer(0),
			  m_allowedPenetration(btScalar(0)),
			  m_subSimplexCastMaxIterations(MAX_CONVEX_CAST_ITERATIONS),
			  m_subSimplexCastEpsilon(MAX_CONVEX_CAST_EPSILON)
		{
		}

		virtual ~CastResult(){};

		btTransform m_hitTransformA;
		btTransform m_hitTransformB;
		btVector3 m_normal;
		btVector3 m_hitPoint;
		btScalar m_fraction;  
		btIDebugDraw* m_debugDrawer;
		btScalar m_allowedPenetration;
		
		int m_subSimplexCastMaxIterations;
		btScalar m_subSimplexCastEpsilon;

	};

	
	virtual bool calcTimeOfImpact(
		const btTransform& fromA,
		const btTransform& toA,
		const btTransform& fromB,
		const btTransform& toB,
		CastResult& result) = 0;
};

#endif  
