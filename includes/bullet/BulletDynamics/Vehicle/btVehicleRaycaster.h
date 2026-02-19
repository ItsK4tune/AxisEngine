
#ifndef BT_VEHICLE_RAYCASTER_H
#define BT_VEHICLE_RAYCASTER_H

#include "LinearMath/btVector3.h"


struct btVehicleRaycaster
{
	virtual ~btVehicleRaycaster()
	{
	}
	struct btVehicleRaycasterResult
	{
		btVehicleRaycasterResult() : m_distFraction(btScalar(-1.)){};
		btVector3 m_hitPointInWorld;
		btVector3 m_hitNormalInWorld;
		btScalar m_distFraction;
	};

	virtual void* castRay(const btVector3& from, const btVector3& to, btVehicleRaycasterResult& result) = 0;
};

#endif  
