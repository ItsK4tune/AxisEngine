

#include "btCylinderShape.h"

btCylinderShape::btCylinderShape(const btVector3& halfExtents)
	: btConvexInternalShape(),
	  m_upAxis(1)
{
	btVector3 margin(getMargin(), getMargin(), getMargin());
	m_implicitShapeDimensions = (halfExtents * m_localScaling) - margin;

	setSafeMargin(halfExtents);

	m_shapeType = CYLINDER_SHAPE_PROXYTYPE;
}

btCylinderShapeX::btCylinderShapeX(const btVector3& halfExtents)
	: btCylinderShape(halfExtents)
{
	m_upAxis = 0;
}

btCylinderShapeZ::btCylinderShapeZ(const btVector3& halfExtents)
	: btCylinderShape(halfExtents)
{
	m_upAxis = 2;
}

void btCylinderShape::getAabb(const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const
{
	btTransformAabb(getHalfExtentsWithoutMargin(), getMargin(), t, aabbMin, aabbMax);
}

void btCylinderShape::calculateLocalInertia(btScalar mass, btVector3& inertia) const
{


#ifndef USE_BOX_INERTIA_APPROXIMATION

	

	btScalar radius2;                                    
	btScalar height2;                                    
	btVector3 halfExtents = getHalfExtentsWithMargin();  
	btScalar div12 = mass / 12.f;
	btScalar div4 = mass / 4.f;
	btScalar div2 = mass / 2.f;
	int idxRadius, idxHeight;

	switch (m_upAxis)  
	{
		case 0:  
			idxRadius = 1;
			idxHeight = 0;
			break;
		case 2:  
			idxRadius = 0;
			idxHeight = 2;
			break;
		default:  
			idxRadius = 0;
			idxHeight = 1;
	}

	
	radius2 = halfExtents[idxRadius] * halfExtents[idxRadius];
	height2 = btScalar(4.) * halfExtents[idxHeight] * halfExtents[idxHeight];

	
	btScalar t1 = div12 * height2 + div4 * radius2;
	btScalar t2 = div2 * radius2;

	switch (m_upAxis)  
	{
		case 0:  
			inertia.setValue(t2, t1, t1);
			break;
		case 2:  
			inertia.setValue(t1, t1, t2);
			break;
		default:  
			inertia.setValue(t1, t2, t1);
	}
#else   
	
	btVector3 halfExtents = getHalfExtentsWithMargin();

	btScalar lx = btScalar(2.) * (halfExtents.x());
	btScalar ly = btScalar(2.) * (halfExtents.y());
	btScalar lz = btScalar(2.) * (halfExtents.z());

	inertia.setValue(mass / (btScalar(12.0)) * (ly * ly + lz * lz),
					 mass / (btScalar(12.0)) * (lx * lx + lz * lz),
					 mass / (btScalar(12.0)) * (lx * lx + ly * ly));
#endif  
}

SIMD_FORCE_INLINE btVector3 CylinderLocalSupportX(const btVector3& halfExtents, const btVector3& v)
{
	const int cylinderUpAxis = 0;
	const int XX = 1;
	const int YY = 0;
	const int ZZ = 2;

	
	

	btScalar radius = halfExtents[XX];
	btScalar halfHeight = halfExtents[cylinderUpAxis];

	btVector3 tmp;
	btScalar d;

	btScalar s = btSqrt(v[XX] * v[XX] + v[ZZ] * v[ZZ]);
	if (s != btScalar(0.0))
	{
		d = radius / s;
		tmp[XX] = v[XX] * d;
		tmp[YY] = v[YY] < 0.0 ? -halfHeight : halfHeight;
		tmp[ZZ] = v[ZZ] * d;
		return tmp;
	}
	else
	{
		tmp[XX] = radius;
		tmp[YY] = v[YY] < 0.0 ? -halfHeight : halfHeight;
		tmp[ZZ] = btScalar(0.0);
		return tmp;
	}
}

inline btVector3 CylinderLocalSupportY(const btVector3& halfExtents, const btVector3& v)
{
	const int cylinderUpAxis = 1;
	const int XX = 0;
	const int YY = 1;
	const int ZZ = 2;

	btScalar radius = halfExtents[XX];
	btScalar halfHeight = halfExtents[cylinderUpAxis];

	btVector3 tmp;
	btScalar d;

	btScalar s = btSqrt(v[XX] * v[XX] + v[ZZ] * v[ZZ]);
	if (s != btScalar(0.0))
	{
		d = radius / s;
		tmp[XX] = v[XX] * d;
		tmp[YY] = v[YY] < 0.0 ? -halfHeight : halfHeight;
		tmp[ZZ] = v[ZZ] * d;
		return tmp;
	}
	else
	{
		tmp[XX] = radius;
		tmp[YY] = v[YY] < 0.0 ? -halfHeight : halfHeight;
		tmp[ZZ] = btScalar(0.0);
		return tmp;
	}
}

inline btVector3 CylinderLocalSupportZ(const btVector3& halfExtents, const btVector3& v)
{
	const int cylinderUpAxis = 2;
	const int XX = 0;
	const int YY = 2;
	const int ZZ = 1;

	
	

	btScalar radius = halfExtents[XX];
	btScalar halfHeight = halfExtents[cylinderUpAxis];

	btVector3 tmp;
	btScalar d;

	btScalar s = btSqrt(v[XX] * v[XX] + v[ZZ] * v[ZZ]);
	if (s != btScalar(0.0))
	{
		d = radius / s;
		tmp[XX] = v[XX] * d;
		tmp[YY] = v[YY] < 0.0 ? -halfHeight : halfHeight;
		tmp[ZZ] = v[ZZ] * d;
		return tmp;
	}
	else
	{
		tmp[XX] = radius;
		tmp[YY] = v[YY] < 0.0 ? -halfHeight : halfHeight;
		tmp[ZZ] = btScalar(0.0);
		return tmp;
	}
}

btVector3 btCylinderShapeX::localGetSupportingVertexWithoutMargin(const btVector3& vec) const
{
	return CylinderLocalSupportX(getHalfExtentsWithoutMargin(), vec);
}

btVector3 btCylinderShapeZ::localGetSupportingVertexWithoutMargin(const btVector3& vec) const
{
	return CylinderLocalSupportZ(getHalfExtentsWithoutMargin(), vec);
}
btVector3 btCylinderShape::localGetSupportingVertexWithoutMargin(const btVector3& vec) const
{
	return CylinderLocalSupportY(getHalfExtentsWithoutMargin(), vec);
}

void btCylinderShape::batchedUnitVectorGetSupportingVertexWithoutMargin(const btVector3* vectors, btVector3* supportVerticesOut, int numVectors) const
{
	for (int i = 0; i < numVectors; i++)
	{
		supportVerticesOut[i] = CylinderLocalSupportY(getHalfExtentsWithoutMargin(), vectors[i]);
	}
}

void btCylinderShapeZ::batchedUnitVectorGetSupportingVertexWithoutMargin(const btVector3* vectors, btVector3* supportVerticesOut, int numVectors) const
{
	for (int i = 0; i < numVectors; i++)
	{
		supportVerticesOut[i] = CylinderLocalSupportZ(getHalfExtentsWithoutMargin(), vectors[i]);
	}
}

void btCylinderShapeX::batchedUnitVectorGetSupportingVertexWithoutMargin(const btVector3* vectors, btVector3* supportVerticesOut, int numVectors) const
{
	for (int i = 0; i < numVectors; i++)
	{
		supportVerticesOut[i] = CylinderLocalSupportX(getHalfExtentsWithoutMargin(), vectors[i]);
	}
}
