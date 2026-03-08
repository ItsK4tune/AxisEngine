

#include "btHeightfieldTerrainShape.h"

#include "LinearMath/btTransformUtil.h"

btHeightfieldTerrainShape::btHeightfieldTerrainShape(
	int heightStickWidth, int heightStickLength,
	const float* heightfieldData, btScalar minHeight, btScalar maxHeight,
	int upAxis, bool flipQuadEdges)
	: m_userValue3(0), m_triangleInfoMap(0)
{
	initialize(heightStickWidth, heightStickLength, heightfieldData,
			   1, minHeight, maxHeight, upAxis, PHY_FLOAT,
			   flipQuadEdges);
}

btHeightfieldTerrainShape::btHeightfieldTerrainShape(
	int heightStickWidth, int heightStickLength, const double* heightfieldData,
	btScalar minHeight, btScalar maxHeight, int upAxis, bool flipQuadEdges)
	: m_userValue3(0), m_triangleInfoMap(0)
{
	initialize(heightStickWidth, heightStickLength, heightfieldData,
			   1, minHeight, maxHeight, upAxis, PHY_DOUBLE,
			   flipQuadEdges);
}

btHeightfieldTerrainShape::btHeightfieldTerrainShape(
	int heightStickWidth, int heightStickLength, const short* heightfieldData, btScalar heightScale,
	btScalar minHeight, btScalar maxHeight, int upAxis, bool flipQuadEdges)
	: m_userValue3(0), m_triangleInfoMap(0)
{
	initialize(heightStickWidth, heightStickLength, heightfieldData,
			   heightScale, minHeight, maxHeight, upAxis, PHY_SHORT,
			   flipQuadEdges);
}

btHeightfieldTerrainShape::btHeightfieldTerrainShape(
	int heightStickWidth, int heightStickLength, const unsigned char* heightfieldData, btScalar heightScale,
	btScalar minHeight, btScalar maxHeight, int upAxis, bool flipQuadEdges)
	: m_userValue3(0), m_triangleInfoMap(0)
{
	initialize(heightStickWidth, heightStickLength, heightfieldData,
			   heightScale, minHeight, maxHeight, upAxis, PHY_UCHAR,
			   flipQuadEdges);
}

btHeightfieldTerrainShape::btHeightfieldTerrainShape(
	int heightStickWidth, int heightStickLength, const void* heightfieldData,
	btScalar heightScale, btScalar minHeight, btScalar maxHeight, int upAxis,
	PHY_ScalarType hdt, bool flipQuadEdges)
	:m_userValue3(0),
	m_triangleInfoMap(0)
{
	
#ifdef BT_USE_DOUBLE_PRECISION
	if (hdt == PHY_FLOAT) hdt = PHY_DOUBLE;
#endif
	initialize(heightStickWidth, heightStickLength, heightfieldData,
			   heightScale, minHeight, maxHeight, upAxis, hdt,
			   flipQuadEdges);
}

btHeightfieldTerrainShape::btHeightfieldTerrainShape(int heightStickWidth, int heightStickLength, const void* heightfieldData, btScalar maxHeight, int upAxis, bool useFloatData, bool flipQuadEdges)
	:	m_userValue3(0),
	m_triangleInfoMap(0)
{
	
	
	PHY_ScalarType hdt = (useFloatData) ? PHY_FLOAT : PHY_UCHAR;
#ifdef BT_USE_DOUBLE_PRECISION
	if (hdt == PHY_FLOAT) hdt = PHY_DOUBLE;
#endif
	btScalar minHeight = 0.0f;

	
	
	btScalar heightScale = maxHeight / 65535;

	initialize(heightStickWidth, heightStickLength, heightfieldData,
			   heightScale, minHeight, maxHeight, upAxis, hdt,
			   flipQuadEdges);
}

void btHeightfieldTerrainShape::initialize(
	int heightStickWidth, int heightStickLength, const void* heightfieldData,
	btScalar heightScale, btScalar minHeight, btScalar maxHeight, int upAxis,
	PHY_ScalarType hdt, bool flipQuadEdges)
{
	
	btAssert(heightStickWidth > 1);   
	btAssert(heightStickLength > 1);  
	btAssert(heightfieldData);        
	
	btAssert(minHeight <= maxHeight);                                    
	btAssert(upAxis >= 0 && upAxis < 3);                                 
	btAssert(hdt != PHY_UCHAR || hdt != PHY_FLOAT || hdt != PHY_DOUBLE || hdt != PHY_SHORT);  

	
	m_shapeType = TERRAIN_SHAPE_PROXYTYPE;
	m_heightStickWidth = heightStickWidth;
	m_heightStickLength = heightStickLength;
	m_minHeight = minHeight;
	m_maxHeight = maxHeight;
	m_width = (btScalar)(heightStickWidth - 1);
	m_length = (btScalar)(heightStickLength - 1);
	m_heightScale = heightScale;
	m_heightfieldDataUnknown = heightfieldData;
	m_heightDataType = hdt;
	m_flipQuadEdges = flipQuadEdges;
	m_useDiamondSubdivision = false;
	m_useZigzagSubdivision = false;
	m_flipTriangleWinding = false;
	m_upAxis = upAxis;
	m_localScaling.setValue(btScalar(1.), btScalar(1.), btScalar(1.));
	
	m_vboundsChunkSize = 0;
	m_vboundsGridWidth = 0;
	m_vboundsGridLength = 0;

	
	switch (m_upAxis)
	{
		case 0:
		{
			m_localAabbMin.setValue(m_minHeight, 0, 0);
			m_localAabbMax.setValue(m_maxHeight, m_width, m_length);
			break;
		}
		case 1:
		{
			m_localAabbMin.setValue(0, m_minHeight, 0);
			m_localAabbMax.setValue(m_width, m_maxHeight, m_length);
			break;
		};
		case 2:
		{
			m_localAabbMin.setValue(0, 0, m_minHeight);
			m_localAabbMax.setValue(m_width, m_length, m_maxHeight);
			break;
		}
		default:
		{
			
			btAssert(0);  
		}
	}

	
	m_localOrigin = btScalar(0.5) * (m_localAabbMin + m_localAabbMax);
}

btHeightfieldTerrainShape::~btHeightfieldTerrainShape()
{
	clearAccelerator();
}

void btHeightfieldTerrainShape::getAabb(const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const
{
	btVector3 halfExtents = (m_localAabbMax - m_localAabbMin) * m_localScaling * btScalar(0.5);

	btVector3 localOrigin(0, 0, 0);
	localOrigin[m_upAxis] = (m_minHeight + m_maxHeight) * btScalar(0.5);
	localOrigin *= m_localScaling;

	btMatrix3x3 abs_b = t.getBasis().absolute();
	btVector3 center = t.getOrigin();
	btVector3 extent = halfExtents.dot3(abs_b[0], abs_b[1], abs_b[2]);
	extent += btVector3(getMargin(), getMargin(), getMargin());

	aabbMin = center - extent;
	aabbMax = center + extent;
}




btScalar
btHeightfieldTerrainShape::getRawHeightFieldValue(int x, int y) const
{
	btScalar val = 0.f;
	switch (m_heightDataType)
	{
		case PHY_FLOAT:
		{
			val = m_heightfieldDataFloat[(y * m_heightStickWidth) + x];
			break;
		}

		case PHY_DOUBLE:
		{
			val = m_heightfieldDataDouble[(y * m_heightStickWidth) + x];
			break;
		}

		case PHY_UCHAR:
		{
			unsigned char heightFieldValue = m_heightfieldDataUnsignedChar[(y * m_heightStickWidth) + x];
			val = heightFieldValue * m_heightScale;
			break;
		}

		case PHY_SHORT:
		{
			short hfValue = m_heightfieldDataShort[(y * m_heightStickWidth) + x];
			val = hfValue * m_heightScale;
			break;
		}

		default:
		{
			btAssert(!"Bad m_heightDataType");
		}
	}

	return val;
}


void btHeightfieldTerrainShape::getVertex(int x, int y, btVector3& Vertex) const
{
	btAssert(x >= 0);
	btAssert(y >= 0);
	btAssert(x < m_heightStickWidth);
	btAssert(y < m_heightStickLength);

	btScalar height = getRawHeightFieldValue(x, y);

	switch (m_upAxis)
	{
		case 0:
		{
			Vertex.setValue(
				height - m_localOrigin.getX(),
				(-m_width / btScalar(2.0)) + x,
				(-m_length / btScalar(2.0)) + y);
			break;
		}
		case 1:
		{
			Vertex.setValue(
				(-m_width / btScalar(2.0)) + x,
				height - m_localOrigin.getY(),
				(-m_length / btScalar(2.0)) + y);
			break;
		};
		case 2:
		{
			Vertex.setValue(
				(-m_width / btScalar(2.0)) + x,
				(-m_length / btScalar(2.0)) + y,
				height - m_localOrigin.getZ());
			break;
		}
		default:
		{
			
			btAssert(0);
		}
	}

	Vertex *= m_localScaling;
}

static inline int
getQuantized(
	btScalar x)
{
	if (x < 0.0)
	{
		return (int)(x - 0.5);
	}
	return (int)(x + 0.5);
}



static btHeightfieldTerrainShape::Range minmaxRange(btScalar a, btScalar b, btScalar c)
{
	if (a > b)
	{
		if (b > c)
			return btHeightfieldTerrainShape::Range(c, a);
		else if (a > c)
			return btHeightfieldTerrainShape::Range(b, a);
		else
			return btHeightfieldTerrainShape::Range(b, c);
	}
	else
	{
		if (a > c)
			return btHeightfieldTerrainShape::Range(c, b);
		else if (b > c)
			return btHeightfieldTerrainShape::Range(a, b);
		else
			return btHeightfieldTerrainShape::Range(a, c);
	}
}



void btHeightfieldTerrainShape::quantizeWithClamp(int* out, const btVector3& point, int ) const
{
	btVector3 clampedPoint(point);
	clampedPoint.setMax(m_localAabbMin);
	clampedPoint.setMin(m_localAabbMax);

	out[0] = getQuantized(clampedPoint.getX());
	out[1] = getQuantized(clampedPoint.getY());
	out[2] = getQuantized(clampedPoint.getZ());
}



void btHeightfieldTerrainShape::processAllTriangles(btTriangleCallback* callback, const btVector3& aabbMin, const btVector3& aabbMax) const
{
	
	btVector3 localAabbMin = aabbMin * btVector3(1.f / m_localScaling[0], 1.f / m_localScaling[1], 1.f / m_localScaling[2]);
	btVector3 localAabbMax = aabbMax * btVector3(1.f / m_localScaling[0], 1.f / m_localScaling[1], 1.f / m_localScaling[2]);

	
	localAabbMin += m_localOrigin;
	localAabbMax += m_localOrigin;

	
	int quantizedAabbMin[3];
	int quantizedAabbMax[3];
	quantizeWithClamp(quantizedAabbMin, localAabbMin, 0);
	quantizeWithClamp(quantizedAabbMax, localAabbMax, 1);

	
	
	for (int i = 0; i < 3; ++i)
	{
		quantizedAabbMin[i]--;
		quantizedAabbMax[i]++;
	}

	int startX = 0;
	int endX = m_heightStickWidth - 1;
	int startJ = 0;
	int endJ = m_heightStickLength - 1;

	switch (m_upAxis)
	{
		case 0:
		{
			if (quantizedAabbMin[1] > startX)
				startX = quantizedAabbMin[1];
			if (quantizedAabbMax[1] < endX)
				endX = quantizedAabbMax[1];
			if (quantizedAabbMin[2] > startJ)
				startJ = quantizedAabbMin[2];
			if (quantizedAabbMax[2] < endJ)
				endJ = quantizedAabbMax[2];
			break;
		}
		case 1:
		{
			if (quantizedAabbMin[0] > startX)
				startX = quantizedAabbMin[0];
			if (quantizedAabbMax[0] < endX)
				endX = quantizedAabbMax[0];
			if (quantizedAabbMin[2] > startJ)
				startJ = quantizedAabbMin[2];
			if (quantizedAabbMax[2] < endJ)
				endJ = quantizedAabbMax[2];
			break;
		};
		case 2:
		{
			if (quantizedAabbMin[0] > startX)
				startX = quantizedAabbMin[0];
			if (quantizedAabbMax[0] < endX)
				endX = quantizedAabbMax[0];
			if (quantizedAabbMin[1] > startJ)
				startJ = quantizedAabbMin[1];
			if (quantizedAabbMax[1] < endJ)
				endJ = quantizedAabbMax[1];
			break;
		}
		default:
		{
			
			btAssert(0);
		}
	}

	
	
	const Range aabbUpRange(aabbMin[m_upAxis], aabbMax[m_upAxis]);
	for (int j = startJ; j < endJ; j++)
	{
		for (int x = startX; x < endX; x++)
		{
			btVector3 vertices[3];
			int indices[3] = { 0, 1, 2 };
			if (m_flipTriangleWinding)
			{
				indices[0] = 2;
				indices[2] = 0;
			}

			if (m_flipQuadEdges || (m_useDiamondSubdivision && !((j + x) & 1)) || (m_useZigzagSubdivision && !(j & 1)))
			{
				getVertex(x, j, vertices[indices[0]]);
				getVertex(x, j + 1, vertices[indices[1]]);
				getVertex(x + 1, j + 1, vertices[indices[2]]);

				
				Range upRange = minmaxRange(vertices[0][m_upAxis], vertices[1][m_upAxis], vertices[2][m_upAxis]);

				if (upRange.overlaps(aabbUpRange))
					callback->processTriangle(vertices, 2 * x, j);
			
				

				
				vertices[indices[1]] = vertices[indices[2]];

				getVertex(x + 1, j, vertices[indices[2]]);
				upRange.min = btMin(upRange.min, vertices[indices[2]][m_upAxis]);
				upRange.max = btMax(upRange.max, vertices[indices[2]][m_upAxis]);

				if (upRange.overlaps(aabbUpRange))
					callback->processTriangle(vertices, 2 * x + 1, j);
			}
			else
			{
				getVertex(x, j, vertices[indices[0]]);
				getVertex(x, j + 1, vertices[indices[1]]);
				getVertex(x + 1, j, vertices[indices[2]]);

				
				Range upRange = minmaxRange(vertices[0][m_upAxis], vertices[1][m_upAxis], vertices[2][m_upAxis]);

				if (upRange.overlaps(aabbUpRange))
					callback->processTriangle(vertices, 2 * x, j);

				

				
				vertices[indices[0]] = vertices[indices[2]];

				getVertex(x + 1, j + 1, vertices[indices[2]]);
				upRange.min = btMin(upRange.min, vertices[indices[2]][m_upAxis]);
				upRange.max = btMax(upRange.max, vertices[indices[2]][m_upAxis]);

				if (upRange.overlaps(aabbUpRange))
					callback->processTriangle(vertices, 2 * x + 1, j);
			}
		}
	}
}

void btHeightfieldTerrainShape::calculateLocalInertia(btScalar, btVector3& inertia) const
{
	

	inertia.setValue(btScalar(0.), btScalar(0.), btScalar(0.));
}

void btHeightfieldTerrainShape::setLocalScaling(const btVector3& scaling)
{
	m_localScaling = scaling;
}
const btVector3& btHeightfieldTerrainShape::getLocalScaling() const
{
	return m_localScaling;
}

namespace
{
	struct GridRaycastState
	{
		int x;  
		int z;
		int prev_x;  
		int prev_z;
		btScalar param;      
		btScalar prevParam;  
		btScalar maxDistanceFlat;
		btScalar maxDistance3d;
	};
}





template <typename Action_T>
void gridRaycast(Action_T& quadAction, const btVector3& beginPos, const btVector3& endPos, int indices[3])
{
	GridRaycastState rs;
	rs.maxDistance3d = beginPos.distance(endPos);
	if (rs.maxDistance3d < 0.0001)
	{
		
		return;
	}
	

	btScalar rayDirectionFlatX = endPos[indices[0]] - beginPos[indices[0]];
	btScalar rayDirectionFlatZ = endPos[indices[2]] - beginPos[indices[2]];
	rs.maxDistanceFlat = btSqrt(rayDirectionFlatX * rayDirectionFlatX + rayDirectionFlatZ * rayDirectionFlatZ);

	if (rs.maxDistanceFlat < 0.0001)
	{
		
		rayDirectionFlatX = 0;
		rayDirectionFlatZ = 0;
	}
	else
	{
		rayDirectionFlatX /= rs.maxDistanceFlat;
		rayDirectionFlatZ /= rs.maxDistanceFlat;
	}

	const int xiStep = rayDirectionFlatX > 0 ? 1 : rayDirectionFlatX < 0 ? -1 : 0;
	const int ziStep = rayDirectionFlatZ > 0 ? 1 : rayDirectionFlatZ < 0 ? -1 : 0;

	const float infinite = 9999999;
	const btScalar paramDeltaX = xiStep != 0 ? 1.f / btFabs(rayDirectionFlatX) : infinite;
	const btScalar paramDeltaZ = ziStep != 0 ? 1.f / btFabs(rayDirectionFlatZ) : infinite;

	
	btScalar paramCrossX;  
	btScalar paramCrossZ;  

	
	
	if (xiStep != 0)
	{
		if (xiStep == 1)
		{
			paramCrossX = (ceil(beginPos[indices[0]]) - beginPos[indices[0]]) * paramDeltaX;
		}
		else
		{
			paramCrossX = (beginPos[indices[0]] - floor(beginPos[indices[0]])) * paramDeltaX;
		}
	}
	else
	{
		paramCrossX = infinite;  
	}

	
	if (ziStep != 0)
	{
		if (ziStep == 1)
		{
			paramCrossZ = (ceil(beginPos[indices[2]]) - beginPos[indices[2]]) * paramDeltaZ;
		}
		else
		{
			paramCrossZ = (beginPos[indices[2]] - floor(beginPos[indices[2]])) * paramDeltaZ;
		}
	}
	else
	{
		paramCrossZ = infinite;  
	}

	rs.x = static_cast<int>(floor(beginPos[indices[0]]));
	rs.z = static_cast<int>(floor(beginPos[indices[2]]));

	
	if (paramCrossX == 0.0)
	{
		paramCrossX += paramDeltaX;
		
		
		if (xiStep == -1)
		{
			rs.x -= 1;
		}
	}

	if (paramCrossZ == 0.0)
	{
		paramCrossZ += paramDeltaZ;
		if (ziStep == -1)
			rs.z -= 1;
	}

	rs.prev_x = rs.x;
	rs.prev_z = rs.z;
	rs.param = 0;

	while (true)
	{
		rs.prev_x = rs.x;
		rs.prev_z = rs.z;
		rs.prevParam = rs.param;

		if (paramCrossX < paramCrossZ)
		{
			
			rs.x += xiStep;
			
			
			rs.param = paramCrossX;
			paramCrossX += paramDeltaX;
		}
		else
		{
			
			rs.z += ziStep;
			rs.param = paramCrossZ;
			paramCrossZ += paramDeltaZ;
		}

		if (rs.param > rs.maxDistanceFlat)
		{
			rs.param = rs.maxDistanceFlat;
			quadAction(rs);
			break;
		}
		else
		{
			quadAction(rs);
		}
	}
}

struct ProcessTrianglesAction
{
	const btHeightfieldTerrainShape* shape;
	bool flipQuadEdges;
	bool useDiamondSubdivision;
	int width;
	int length;
	btTriangleCallback* callback;

	void exec(int x, int z) const
	{
		if (x < 0 || z < 0 || x >= width || z >= length)
		{
			return;
		}

		btVector3 vertices[3];

		

		
		if (flipQuadEdges || (useDiamondSubdivision && (((z + x) & 1) > 0)))
		{
			
			shape->getVertex(x, z, vertices[0]);
			shape->getVertex(x + 1, z, vertices[1]);
			shape->getVertex(x + 1, z + 1, vertices[2]);
			callback->processTriangle(vertices, x, z);

			
			shape->getVertex(x, z, vertices[0]);
			shape->getVertex(x + 1, z + 1, vertices[1]);
			shape->getVertex(x, z + 1, vertices[2]);
			callback->processTriangle(vertices, x, z);
		}
		else
		{
			
			shape->getVertex(x, z, vertices[0]);
			shape->getVertex(x, z + 1, vertices[1]);
			shape->getVertex(x + 1, z, vertices[2]);
			callback->processTriangle(vertices, x, z);

			
			shape->getVertex(x + 1, z, vertices[0]);
			shape->getVertex(x, z + 1, vertices[1]);
			shape->getVertex(x + 1, z + 1, vertices[2]);
			callback->processTriangle(vertices, x, z);
		}
	}

	void operator()(const GridRaycastState& bs) const
	{
		exec(bs.prev_x, bs.prev_z);
	}
};

struct ProcessVBoundsAction
{
	const btAlignedObjectArray<btHeightfieldTerrainShape::Range>& vbounds;
	int width;
	int length;
	int chunkSize;

	btVector3 rayBegin;
	btVector3 rayEnd;
	btVector3 rayDir;

	int* m_indices;
	ProcessTrianglesAction processTriangles;

	ProcessVBoundsAction(const btAlignedObjectArray<btHeightfieldTerrainShape::Range>& bnd, int* indices)
		: vbounds(bnd),
		m_indices(indices)
	{
	}
	void operator()(const GridRaycastState& rs) const
	{
		int x = rs.prev_x;
		int z = rs.prev_z;

		if (x < 0 || z < 0 || x >= width || z >= length)
		{
			return;
		}

		const btHeightfieldTerrainShape::Range chunk = vbounds[x + z * width];

		btVector3 enterPos;
		btVector3 exitPos;

		if (rs.maxDistanceFlat > 0.0001)
		{
			btScalar flatTo3d = chunkSize * rs.maxDistance3d / rs.maxDistanceFlat;
			btScalar enterParam3d = rs.prevParam * flatTo3d;
			btScalar exitParam3d = rs.param * flatTo3d;
			enterPos = rayBegin + rayDir * enterParam3d;
			exitPos = rayBegin + rayDir * exitParam3d;

			
			
			if (enterPos[1] > chunk.max && exitPos[m_indices[1]] > chunk.max)
			{
				return;
			}
			if (enterPos[1] < chunk.min && exitPos[m_indices[1]] < chunk.min)
			{
				return;
			}
		}
		else
		{
			
			
			enterPos = rayBegin;
			exitPos = rayEnd;
		}

		gridRaycast(processTriangles, enterPos, exitPos, m_indices);
		
		
	}
};




void btHeightfieldTerrainShape::performRaycast(btTriangleCallback* callback, const btVector3& raySource, const btVector3& rayTarget) const
{
	
	btVector3 beginPos = raySource / m_localScaling;
	btVector3 endPos = rayTarget / m_localScaling;
	beginPos += m_localOrigin;
	endPos += m_localOrigin;

	ProcessTrianglesAction processTriangles;
	processTriangles.shape = this;
	processTriangles.flipQuadEdges = m_flipQuadEdges;
	processTriangles.useDiamondSubdivision = m_useDiamondSubdivision;
	processTriangles.callback = callback;
	processTriangles.width = m_heightStickWidth - 1;
	processTriangles.length = m_heightStickLength - 1;

	
	int indices[3] = { 0, 1, 2 };
	if (m_upAxis == 2)
	{
		indices[1] = 2;
		indices[2] = 1;
	}
	int iBeginX = static_cast<int>(floor(beginPos[indices[0]]));
	int iBeginZ = static_cast<int>(floor(beginPos[indices[2]]));
	int iEndX = static_cast<int>(floor(endPos[indices[0]]));
	int iEndZ = static_cast<int>(floor(endPos[indices[2]]));

	if (iBeginX == iEndX && iBeginZ == iEndZ)
	{
		
		
		
		processTriangles.exec(iBeginX, iEndZ);
		return;
	}

	

	if (m_vboundsGrid.size()==0)
	{
		
		gridRaycast(processTriangles, beginPos, endPos, &indices[0]);
	}
	else
	{
		btVector3 rayDiff = endPos - beginPos;
		btScalar flatDistance2 = rayDiff[indices[0]] * rayDiff[indices[0]] + rayDiff[indices[2]] * rayDiff[indices[2]];
		if (flatDistance2 < m_vboundsChunkSize * m_vboundsChunkSize)
		{
			
			gridRaycast(processTriangles, beginPos, endPos, &indices[0]);
			return;
		}

		ProcessVBoundsAction processVBounds(m_vboundsGrid, &indices[0]);
		processVBounds.width = m_vboundsGridWidth;
		processVBounds.length = m_vboundsGridLength;
		processVBounds.rayBegin = beginPos;
		processVBounds.rayEnd = endPos;
		processVBounds.rayDir = rayDiff.normalized();
		processVBounds.processTriangles = processTriangles;
		processVBounds.chunkSize = m_vboundsChunkSize;
		
		gridRaycast(processVBounds, beginPos / m_vboundsChunkSize, endPos / m_vboundsChunkSize, indices);
	}
}




void btHeightfieldTerrainShape::buildAccelerator(int chunkSize)
{
	if (chunkSize <= 0)
	{
		clearAccelerator();
		return;
	}

	m_vboundsChunkSize = chunkSize;
	int nChunksX = m_heightStickWidth / chunkSize;
	int nChunksZ = m_heightStickLength / chunkSize;

	if (m_heightStickWidth % chunkSize > 0)
	{
		++nChunksX;  
	}
	if (m_heightStickLength % chunkSize > 0)
	{
		++nChunksZ;
	}

	if (m_vboundsGridWidth != nChunksX || m_vboundsGridLength != nChunksZ)
	{
		clearAccelerator();
		m_vboundsGridWidth = nChunksX;
		m_vboundsGridLength = nChunksZ;
	}

	if (nChunksX == 0 || nChunksZ == 0)
	{
		return;
	}

	
	m_vboundsGrid.resize(nChunksX * nChunksZ);
	
	
	for (int cz = 0; cz < nChunksZ; ++cz)
	{
		int z0 = cz * chunkSize;

		for (int cx = 0; cx < nChunksX; ++cx)
		{
			int x0 = cx * chunkSize;

			Range r;

			r.min = getRawHeightFieldValue(x0, z0);
			r.max = r.min;

			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			for (int z = z0; z < z0 + chunkSize + 1; ++z)
			{
				if (z >= m_heightStickLength)
				{
					continue;
				}

				for (int x = x0; x < x0 + chunkSize + 1; ++x)
				{
					if (x >= m_heightStickWidth)
					{
						continue;
					}

					btScalar height = getRawHeightFieldValue(x, z);

					if (height < r.min)
					{
						r.min = height;
					}
					else if (height > r.max)
					{
						r.max = height;
					}
				}
			}

			m_vboundsGrid[cx + cz * nChunksX] = r;
		}
	}
}

void btHeightfieldTerrainShape::clearAccelerator()
{
	m_vboundsGrid.clear();
}