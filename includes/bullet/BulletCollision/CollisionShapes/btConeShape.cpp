

#include "btConeShape.h"

btConeShape::btConeShape(btScalar radius, btScalar height) : btConvexInternalShape(),
															 m_radius(radius),
															 m_height(height)
{
	m_shapeType = CONE_SHAPE_PROXYTYPE;
	setConeUpIndex(1);
	btVector3 halfExtents;
	m_sinAngle = (m_radius / btSqrt(m_radius * m_radius + m_height * m_height));
}

btConeShapeZ::btConeShapeZ(btScalar radius, btScalar height) : btConeShape(radius, height)
{
	setConeUpIndex(2);
}

btConeShapeX::btConeShapeX(btScalar radius, btScalar height) : btConeShape(radius, height)
{
	setConeUpIndex(0);
}


void btConeShape::setConeUpIndex(int upIndex)
{
	switch (upIndex)
	{
		case 0:
			m_coneIndices[0] = 1;
			m_coneIndices[1] = 0;
			m_coneIndices[2] = 2;
			break;
		case 1:
			m_coneIndices[0] = 0;
			m_coneIndices[1] = 1;
			m_coneIndices[2] = 2;
			break;
		case 2:
			m_coneIndices[0] = 0;
			m_coneIndices[1] = 2;
			m_coneIndices[2] = 1;
			break;
		default:
			btAssert(0);
	};

	m_implicitShapeDimensions[m_coneIndices[0]] = m_radius;
	m_implicitShapeDimensions[m_coneIndices[1]] = m_height;
	m_implicitShapeDimensions[m_coneIndices[2]] = m_radius;
}

btVector3 btConeShape::coneLocalSupport(const btVector3& v) const
{
	btScalar halfHeight = m_height * btScalar(0.5);

	if (v[m_coneIndices[1]] > v.length() * m_sinAngle)
	{
		btVector3 tmp;

		tmp[m_coneIndices[0]] = btScalar(0.);
		tmp[m_coneIndices[1]] = halfHeight;
		tmp[m_coneIndices[2]] = btScalar(0.);
		return tmp;
	}
	else
	{
		btScalar s = btSqrt(v[m_coneIndices[0]] * v[m_coneIndices[0]] + v[m_coneIndices[2]] * v[m_coneIndices[2]]);
		if (s > SIMD_EPSILON)
		{
			btScalar d = m_radius / s;
			btVector3 tmp;
			tmp[m_coneIndices[0]] = v[m_coneIndices[0]] * d;
			tmp[m_coneIndices[1]] = -halfHeight;
			tmp[m_coneIndices[2]] = v[m_coneIndices[2]] * d;
			return tmp;
		}
		else
		{
			btVector3 tmp;
			tmp[m_coneIndices[0]] = btScalar(0.);
			tmp[m_coneIndices[1]] = -halfHeight;
			tmp[m_coneIndices[2]] = btScalar(0.);
			return tmp;
		}
	}
}

btVector3 btConeShape::localGetSupportingVertexWithoutMargin(const btVector3& vec) const
{
	return coneLocalSupport(vec);
}

void btConeShape::batchedUnitVectorGetSupportingVertexWithoutMargin(const btVector3* vectors, btVector3* supportVerticesOut, int numVectors) const
{
	for (int i = 0; i < numVectors; i++)
	{
		const btVector3& vec = vectors[i];
		supportVerticesOut[i] = coneLocalSupport(vec);
	}
}

btVector3 btConeShape::localGetSupportingVertex(const btVector3& vec) const
{
	btVector3 supVertex = coneLocalSupport(vec);
	if (getMargin() != btScalar(0.))
	{
		btVector3 vecnorm = vec;
		if (vecnorm.length2() < (SIMD_EPSILON * SIMD_EPSILON))
		{
			vecnorm.setValue(btScalar(-1.), btScalar(-1.), btScalar(-1.));
		}
		vecnorm.normalize();
		supVertex += getMargin() * vecnorm;
	}
	return supVertex;
}

void btConeShape::setLocalScaling(const btVector3& scaling)
{
	int axis = m_coneIndices[1];
	int r1 = m_coneIndices[0];
	int r2 = m_coneIndices[2];
	m_height *= scaling[axis] / m_localScaling[axis];
	m_radius *= (scaling[r1] / m_localScaling[r1] + scaling[r2] / m_localScaling[r2]) / 2;
	m_sinAngle = (m_radius / btSqrt(m_radius * m_radius + m_height * m_height));
	btConvexInternalShape::setLocalScaling(scaling);
}
