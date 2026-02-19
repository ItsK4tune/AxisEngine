

#include "btEmptyCollisionAlgorithm.h"

btEmptyAlgorithm::btEmptyAlgorithm(const btCollisionAlgorithmConstructionInfo& ci)
	: btCollisionAlgorithm(ci)
{
}

void btEmptyAlgorithm::processCollision(const btCollisionObjectWrapper*, const btCollisionObjectWrapper*, const btDispatcherInfo&, btManifoldResult*)
{
}

btScalar btEmptyAlgorithm::calculateTimeOfImpact(btCollisionObject*, btCollisionObject*, const btDispatcherInfo&, btManifoldResult*)
{
	return btScalar(1.);
}
