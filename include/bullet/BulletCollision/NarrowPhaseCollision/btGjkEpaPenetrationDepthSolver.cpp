

#include "BulletCollision/CollisionShapes/btConvexShape.h"
#include "btGjkEpaPenetrationDepthSolver.h"

#include "BulletCollision/NarrowPhaseCollision/btGjkEpa2.h"

bool btGjkEpaPenetrationDepthSolver::calcPenDepth(btSimplexSolverInterface& simplexSolver,
												  const btConvexShape* pConvexA, const btConvexShape* pConvexB,
												  const btTransform& transformA, const btTransform& transformB,
												  btVector3& v, btVector3& wWitnessOnA, btVector3& wWitnessOnB,
												  class btIDebugDraw* debugDraw)
{
	(void)debugDraw;
	(void)v;
	(void)simplexSolver;

	btVector3 guessVectors[] = {
		btVector3(transformB.getOrigin() - transformA.getOrigin()).safeNormalize(),
		btVector3(transformA.getOrigin() - transformB.getOrigin()).safeNormalize(),
		btVector3(0, 0, 1),
		btVector3(0, 1, 0),
		btVector3(1, 0, 0),
		btVector3(1, 1, 0),
		btVector3(1, 1, 1),
		btVector3(0, 1, 1),
		btVector3(1, 0, 1),
	};

	int numVectors = sizeof(guessVectors) / sizeof(btVector3);

	for (int i = 0; i < numVectors; i++)
	{
		simplexSolver.reset();
		btVector3 guessVector = guessVectors[i];

		btGjkEpaSolver2::sResults results;

		if (btGjkEpaSolver2::Penetration(pConvexA, transformA,
										 pConvexB, transformB,
										 guessVector, results))

		{
			wWitnessOnA = results.witnesses[0];
			wWitnessOnB = results.witnesses[1];
			v = results.normal;
			return true;
		}
		else
		{
			if (btGjkEpaSolver2::Distance(pConvexA, transformA, pConvexB, transformB, guessVector, results))
			{
				wWitnessOnA = results.witnesses[0];
				wWitnessOnB = results.witnesses[1];
				v = results.normal;
				return false;
			}
		}
	}

	
	wWitnessOnA.setValue(0, 0, 0);
	wWitnessOnB.setValue(0, 0, 0);
	v.setValue(0, 0, 0);
	return false;
}
