

#ifndef BT_GJK_EPA_PENETATION_CONVEX_COLLISION_H
#define BT_GJK_EPA_PENETATION_CONVEX_COLLISION_H

#include "LinearMath/btTransform.h"  
#include "btGjkEpa3.h"
#include "btGjkCollisionDescription.h"
#include "BulletCollision/NarrowPhaseCollision/btVoronoiSimplexSolver.h"

template <typename btConvexTemplate>
bool btGjkEpaCalcPenDepth(const btConvexTemplate& a, const btConvexTemplate& b,
						  const btGjkCollisionDescription& colDesc,
						  btVector3& v, btVector3& wWitnessOnA, btVector3& wWitnessOnB)
{
	(void)v;

	

	btVector3 guessVector(b.getWorldTransform().getOrigin() - a.getWorldTransform().getOrigin());  

	btGjkEpaSolver3::sResults results;

	if (btGjkEpaSolver3_Penetration(a, b, guessVector, results))

	{
		
		
		wWitnessOnA = results.witnesses[0];
		wWitnessOnB = results.witnesses[1];
		v = results.normal;
		return true;
	}
	else
	{
		if (btGjkEpaSolver3_Distance(a, b, guessVector, results))
		{
			wWitnessOnA = results.witnesses[0];
			wWitnessOnB = results.witnesses[1];
			v = results.normal;
			return false;
		}
	}
	return false;
}

template <typename btConvexTemplate, typename btGjkDistanceTemplate>
int btComputeGjkEpaPenetration(const btConvexTemplate& a, const btConvexTemplate& b, const btGjkCollisionDescription& colDesc, btVoronoiSimplexSolver& simplexSolver, btGjkDistanceTemplate* distInfo)
{
	bool m_catchDegeneracies = true;
	btScalar m_cachedSeparatingDistance = 0.f;

	btScalar distance = btScalar(0.);
	btVector3 normalInB(btScalar(0.), btScalar(0.), btScalar(0.));

	btVector3 pointOnA, pointOnB;
	btTransform localTransA = a.getWorldTransform();
	btTransform localTransB = b.getWorldTransform();

	btScalar marginA = a.getMargin();
	btScalar marginB = b.getMargin();

	int m_curIter = 0;
	int gGjkMaxIter = colDesc.m_maxGjkIterations;  
	btVector3 m_cachedSeparatingAxis = colDesc.m_firstDir;

	bool isValid = false;
	bool checkSimplex = false;
	bool checkPenetration = true;
	int m_degenerateSimplex = 0;

	int m_lastUsedMethod = -1;

	{
		btScalar squaredDistance = BT_LARGE_FLOAT;
		btScalar delta = btScalar(0.);

		btScalar margin = marginA + marginB;

		simplexSolver.reset();

		for (;;)
		
		{
			btVector3 separatingAxisInA = (-m_cachedSeparatingAxis) * localTransA.getBasis();
			btVector3 separatingAxisInB = m_cachedSeparatingAxis * localTransB.getBasis();

			btVector3 pInA = a.getLocalSupportWithoutMargin(separatingAxisInA);
			btVector3 qInB = b.getLocalSupportWithoutMargin(separatingAxisInB);

			btVector3 pWorld = localTransA(pInA);
			btVector3 qWorld = localTransB(qInB);

			btVector3 w = pWorld - qWorld;
			delta = m_cachedSeparatingAxis.dot(w);

			
			if ((delta > btScalar(0.0)) && (delta * delta > squaredDistance * colDesc.m_maximumDistanceSquared))
			{
				m_degenerateSimplex = 10;
				checkSimplex = true;
				
				break;
			}

			
			if (simplexSolver.inSimplex(w))
			{
				m_degenerateSimplex = 1;
				checkSimplex = true;
				break;
			}
			
			btScalar f0 = squaredDistance - delta;
			btScalar f1 = squaredDistance * colDesc.m_gjkRelError2;

			if (f0 <= f1)
			{
				if (f0 <= btScalar(0.))
				{
					m_degenerateSimplex = 2;
				}
				else
				{
					m_degenerateSimplex = 11;
				}
				checkSimplex = true;
				break;
			}

			
			simplexSolver.addVertex(w, pWorld, qWorld);
			btVector3 newCachedSeparatingAxis;

			
			if (!simplexSolver.closest(newCachedSeparatingAxis))
			{
				m_degenerateSimplex = 3;
				checkSimplex = true;
				break;
			}

			if (newCachedSeparatingAxis.length2() < colDesc.m_gjkRelError2)
			{
				m_cachedSeparatingAxis = newCachedSeparatingAxis;
				m_degenerateSimplex = 6;
				checkSimplex = true;
				break;
			}

			btScalar previousSquaredDistance = squaredDistance;
			squaredDistance = newCachedSeparatingAxis.length2();
#if 0
            
            if (squaredDistance>previousSquaredDistance)
            {
                m_degenerateSimplex = 7;
                squaredDistance = previousSquaredDistance;
                checkSimplex = false;
                break;
            }
#endif  

			

			
			if (previousSquaredDistance - squaredDistance <= SIMD_EPSILON * previousSquaredDistance)
			{
				
				checkSimplex = true;
				m_degenerateSimplex = 12;

				break;
			}

			m_cachedSeparatingAxis = newCachedSeparatingAxis;

			
			if (m_curIter++ > gGjkMaxIter)
			{
#if defined(DEBUG) || defined(_DEBUG)

				printf("btGjkPairDetector maxIter exceeded:%i\n", m_curIter);
				printf("sepAxis=(%f,%f,%f), squaredDistance = %f\n",
					   m_cachedSeparatingAxis.getX(),
					   m_cachedSeparatingAxis.getY(),
					   m_cachedSeparatingAxis.getZ(),
					   squaredDistance);
#endif

				break;
			}

			bool check = (!simplexSolver.fullSimplex());
			

			if (!check)
			{
				
				
				m_degenerateSimplex = 13;
				break;
			}
		}

		if (checkSimplex)
		{
			simplexSolver.compute_points(pointOnA, pointOnB);
			normalInB = m_cachedSeparatingAxis;

			btScalar lenSqr = m_cachedSeparatingAxis.length2();

			
			if (lenSqr < 0.0001)
			{
				m_degenerateSimplex = 5;
			}
			if (lenSqr > SIMD_EPSILON * SIMD_EPSILON)
			{
				btScalar rlen = btScalar(1.) / btSqrt(lenSqr);
				normalInB *= rlen;  

				btScalar s = btSqrt(squaredDistance);

				btAssert(s > btScalar(0.0));
				pointOnA -= m_cachedSeparatingAxis * (marginA / s);
				pointOnB += m_cachedSeparatingAxis * (marginB / s);
				distance = ((btScalar(1.) / rlen) - margin);
				isValid = true;

				m_lastUsedMethod = 1;
			}
			else
			{
				m_lastUsedMethod = 2;
			}
		}

		bool catchDegeneratePenetrationCase =
			(m_catchDegeneracies && m_degenerateSimplex && ((distance + margin) < 0.01));

		
		if (checkPenetration && (!isValid || catchDegeneratePenetrationCase))
		{
			

			

			
			btVector3 tmpPointOnA, tmpPointOnB;

			m_cachedSeparatingAxis.setZero();

			bool isValid2 = btGjkEpaCalcPenDepth(a, b,
												 colDesc,
												 m_cachedSeparatingAxis, tmpPointOnA, tmpPointOnB);

			if (isValid2)
			{
				btVector3 tmpNormalInB = tmpPointOnB - tmpPointOnA;
				btScalar lenSqr = tmpNormalInB.length2();
				if (lenSqr <= (SIMD_EPSILON * SIMD_EPSILON))
				{
					tmpNormalInB = m_cachedSeparatingAxis;
					lenSqr = m_cachedSeparatingAxis.length2();
				}

				if (lenSqr > (SIMD_EPSILON * SIMD_EPSILON))
				{
					tmpNormalInB /= btSqrt(lenSqr);
					btScalar distance2 = -(tmpPointOnA - tmpPointOnB).length();
					
					if (!isValid || (distance2 < distance))
					{
						distance = distance2;
						pointOnA = tmpPointOnA;
						pointOnB = tmpPointOnB;
						normalInB = tmpNormalInB;

						isValid = true;
						m_lastUsedMethod = 3;
					}
					else
					{
						m_lastUsedMethod = 8;
					}
				}
				else
				{
					m_lastUsedMethod = 9;
				}
			}
			else

			{
				
				
				
				
				

				if (m_cachedSeparatingAxis.length2() > btScalar(0.))
				{
					btScalar distance2 = (tmpPointOnA - tmpPointOnB).length() - margin;
					
					if (!isValid || (distance2 < distance))
					{
						distance = distance2;
						pointOnA = tmpPointOnA;
						pointOnB = tmpPointOnB;
						pointOnA -= m_cachedSeparatingAxis * marginA;
						pointOnB += m_cachedSeparatingAxis * marginB;
						normalInB = m_cachedSeparatingAxis;
						normalInB.normalize();

						isValid = true;
						m_lastUsedMethod = 6;
					}
					else
					{
						m_lastUsedMethod = 5;
					}
				}
			}
		}
	}

	if (isValid && ((distance < 0) || (distance * distance < colDesc.m_maximumDistanceSquared)))
	{
		m_cachedSeparatingAxis = normalInB;
		m_cachedSeparatingDistance = distance;
		distInfo->m_distance = distance;
		distInfo->m_normalBtoA = normalInB;
		distInfo->m_pointOnB = pointOnB;
		distInfo->m_pointOnA = pointOnB + normalInB * distance;
		return 0;
	}
	return -m_lastUsedMethod;
}

#endif  
