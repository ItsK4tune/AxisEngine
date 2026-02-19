

#if defined(_WIN32) || defined(__i386__)
#define BT_USE_SSE_IN_API
#endif

#include "btMultiSphereShape.h"
#include "BulletCollision/CollisionShapes/btCollisionMargin.h"
#include "LinearMath/btQuaternion.h"
#include "LinearMath/btSerializer.h"

btMultiSphereShape::btMultiSphereShape(const btVector3* positions, const btScalar* radi, int numSpheres)
	: btConvexInternalAabbCachingShape()
{
	m_shapeType = MULTI_SPHERE_SHAPE_PROXYTYPE;
	

	m_localPositionArray.resize(numSpheres);
	m_radiArray.resize(numSpheres);
	for (int i = 0; i < numSpheres; i++)
	{
		m_localPositionArray[i] = positions[i];
		m_radiArray[i] = radi[i];
	}

	recalcLocalAabb();
}

#ifndef MIN
#define MIN(_a, _b) ((_a) < (_b) ? (_a) : (_b))
#endif
btVector3 btMultiSphereShape::localGetSupportingVertexWithoutMargin(const btVector3& vec0) const
{
	btVector3 supVec(0, 0, 0);

	btScalar maxDot(btScalar(-BT_LARGE_FLOAT));

	btVector3 vec = vec0;
	btScalar lenSqr = vec.length2();
	if (lenSqr < (SIMD_EPSILON * SIMD_EPSILON))
	{
		vec.setValue(1, 0, 0);
	}
	else
	{
		btScalar rlen = btScalar(1.) / btSqrt(lenSqr);
		vec *= rlen;
	}

	btVector3 vtx;
	btScalar newDot;

	const btVector3* pos = &m_localPositionArray[0];
	const btScalar* rad = &m_radiArray[0];
	int numSpheres = m_localPositionArray.size();

	for (int k = 0; k < numSpheres; k += 128)
	{
		btVector3 temp[128];
		int inner_count = MIN(numSpheres - k, 128);
		for (long i = 0; i < inner_count; i++)
		{
			temp[i] = (*pos) * m_localScaling + vec * m_localScaling * (*rad) - vec * getMargin();
			pos++;
			rad++;
		}
		long i = vec.maxDot(temp, inner_count, newDot);
		if (newDot > maxDot)
		{
			maxDot = newDot;
			supVec = temp[i];
		}
	}

	return supVec;
}

void btMultiSphereShape::batchedUnitVectorGetSupportingVertexWithoutMargin(const btVector3* vectors, btVector3* supportVerticesOut, int numVectors) const
{
	for (int j = 0; j < numVectors; j++)
	{
		btScalar maxDot(btScalar(-BT_LARGE_FLOAT));

		const btVector3& vec = vectors[j];

		btVector3 vtx;
		btScalar newDot;

		const btVector3* pos = &m_localPositionArray[0];
		const btScalar* rad = &m_radiArray[0];
		int numSpheres = m_localPositionArray.size();

		for (int k = 0; k < numSpheres; k += 128)
		{
			btVector3 temp[128];
			int inner_count = MIN(numSpheres - k, 128);
			for (long i = 0; i < inner_count; i++)
			{
				temp[i] = (*pos) * m_localScaling + vec * m_localScaling * (*rad) - vec * getMargin();
				pos++;
				rad++;
			}
			long i = vec.maxDot(temp, inner_count, newDot);
			if (newDot > maxDot)
			{
				maxDot = newDot;
				supportVerticesOut[j] = temp[i];
			}
		}
	}
}

void btMultiSphereShape::calculateLocalInertia(btScalar mass, btVector3& inertia) const
{
	

	btVector3 localAabbMin, localAabbMax;
	getCachedLocalAabb(localAabbMin, localAabbMax);
	btVector3 halfExtents = (localAabbMax - localAabbMin) * btScalar(0.5);

	btScalar lx = btScalar(2.) * (halfExtents.x());
	btScalar ly = btScalar(2.) * (halfExtents.y());
	btScalar lz = btScalar(2.) * (halfExtents.z());

	inertia.setValue(mass / (btScalar(12.0)) * (ly * ly + lz * lz),
					 mass / (btScalar(12.0)) * (lx * lx + lz * lz),
					 mass / (btScalar(12.0)) * (lx * lx + ly * ly));
}


const char* btMultiSphereShape::serialize(void* dataBuffer, btSerializer* serializer) const
{
	btMultiSphereShapeData* shapeData = (btMultiSphereShapeData*)dataBuffer;
	btConvexInternalShape::serialize(&shapeData->m_convexInternalShapeData, serializer);

	int numElem = m_localPositionArray.size();
	shapeData->m_localPositionArrayPtr = numElem ? (btPositionAndRadius*)serializer->getUniquePointer((void*)&m_localPositionArray[0]) : 0;

	shapeData->m_localPositionArraySize = numElem;
	if (numElem)
	{
		btChunk* chunk = serializer->allocate(sizeof(btPositionAndRadius), numElem);
		btPositionAndRadius* memPtr = (btPositionAndRadius*)chunk->m_oldPtr;
		for (int i = 0; i < numElem; i++, memPtr++)
		{
			m_localPositionArray[i].serializeFloat(memPtr->m_pos);
			memPtr->m_radius = float(m_radiArray[i]);
		}
		serializer->finalizeChunk(chunk, "btPositionAndRadius", BT_ARRAY_CODE, (void*)&m_localPositionArray[0]);
	}

	
	memset(shapeData->m_padding, 0, sizeof(shapeData->m_padding));

	return "btMultiSphereShapeData";
}
