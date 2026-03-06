
#include "BulletCollision/CollisionShapes/btCollisionShape.h"
#include "LinearMath/btSerializer.h"


extern "C"
{
	void btBulletCollisionProbe();

	void btBulletCollisionProbe() {}
}

void btCollisionShape::getBoundingSphere(btVector3& center, btScalar& radius) const
{
	btTransform tr;
	tr.setIdentity();
	btVector3 aabbMin, aabbMax;

	getAabb(tr, aabbMin, aabbMax);

	radius = (aabbMax - aabbMin).length() * btScalar(0.5);
	center = (aabbMin + aabbMax) * btScalar(0.5);
}

btScalar btCollisionShape::getContactBreakingThreshold(btScalar defaultContactThreshold) const
{
	return getAngularMotionDisc() * defaultContactThreshold;
}

btScalar btCollisionShape::getAngularMotionDisc() const
{
	
	btVector3 center;
	btScalar disc;
	getBoundingSphere(center, disc);
	disc += (center).length();
	return disc;
}

void btCollisionShape::calculateTemporalAabb(const btTransform& curTrans, const btVector3& linvel, const btVector3& angvel, btScalar timeStep, btVector3& temporalAabbMin, btVector3& temporalAabbMax) const
{
	
	getAabb(curTrans, temporalAabbMin, temporalAabbMax);

	btScalar temporalAabbMaxx = temporalAabbMax.getX();
	btScalar temporalAabbMaxy = temporalAabbMax.getY();
	btScalar temporalAabbMaxz = temporalAabbMax.getZ();
	btScalar temporalAabbMinx = temporalAabbMin.getX();
	btScalar temporalAabbMiny = temporalAabbMin.getY();
	btScalar temporalAabbMinz = temporalAabbMin.getZ();

	
	btVector3 linMotion = linvel * timeStep;
	
	if (linMotion.x() > btScalar(0.))
		temporalAabbMaxx += linMotion.x();
	else
		temporalAabbMinx += linMotion.x();
	if (linMotion.y() > btScalar(0.))
		temporalAabbMaxy += linMotion.y();
	else
		temporalAabbMiny += linMotion.y();
	if (linMotion.z() > btScalar(0.))
		temporalAabbMaxz += linMotion.z();
	else
		temporalAabbMinz += linMotion.z();

	
	btScalar angularMotion = angvel.length() * getAngularMotionDisc() * timeStep;
	btVector3 angularMotion3d(angularMotion, angularMotion, angularMotion);
	temporalAabbMin = btVector3(temporalAabbMinx, temporalAabbMiny, temporalAabbMinz);
	temporalAabbMax = btVector3(temporalAabbMaxx, temporalAabbMaxy, temporalAabbMaxz);

	temporalAabbMin -= angularMotion3d;
	temporalAabbMax += angularMotion3d;
}


const char* btCollisionShape::serialize(void* dataBuffer, btSerializer* serializer) const
{
	btCollisionShapeData* shapeData = (btCollisionShapeData*)dataBuffer;
	char* name = (char*)serializer->findNameForPointer(this);
	shapeData->m_name = (char*)serializer->getUniquePointer(name);
	if (shapeData->m_name)
	{
		serializer->serializeName(name);
	}
	shapeData->m_shapeType = m_shapeType;

	
	memset(shapeData->m_padding, 0, sizeof(shapeData->m_padding));

	return "btCollisionShapeData";
}

void btCollisionShape::serializeSingleShape(btSerializer* serializer) const
{
	int len = calculateSerializeBufferSize();
	btChunk* chunk = serializer->allocate(len, 1);
	const char* structType = serialize(chunk->m_oldPtr, serializer);
	serializer->finalizeChunk(chunk, structType, BT_SHAPE_CODE, (void*)this);
}
