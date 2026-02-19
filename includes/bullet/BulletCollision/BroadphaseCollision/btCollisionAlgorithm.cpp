

#include "btCollisionAlgorithm.h"
#include "btDispatcher.h"

btCollisionAlgorithm::btCollisionAlgorithm(const btCollisionAlgorithmConstructionInfo& ci)
{
	m_dispatcher = ci.m_dispatcher1;
}
