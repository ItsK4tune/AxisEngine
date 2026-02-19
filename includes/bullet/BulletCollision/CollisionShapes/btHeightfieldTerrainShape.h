

#ifndef BT_HEIGHTFIELD_TERRAIN_SHAPE_H
#define BT_HEIGHTFIELD_TERRAIN_SHAPE_H

#include "btConcaveShape.h"
#include "LinearMath/btAlignedObjectArray.h"



ATTRIBUTE_ALIGNED16(class)
btHeightfieldTerrainShape : public btConcaveShape
{
public:
	struct Range
	{
		Range() {}
		Range(btScalar min, btScalar max) : min(min), max(max) {}

		bool overlaps(const Range& other) const
		{
			return !(min > other.max || max < other.min);
		}

		btScalar min;
		btScalar max;
	};

protected:
	btVector3 m_localAabbMin;
	btVector3 m_localAabbMax;
	btVector3 m_localOrigin;

	
	int m_heightStickWidth;
	int m_heightStickLength;
	btScalar m_minHeight;
	btScalar m_maxHeight;
	btScalar m_width;
	btScalar m_length;
	btScalar m_heightScale;
	union {
		const unsigned char* m_heightfieldDataUnsignedChar;
		const short* m_heightfieldDataShort;
		const float* m_heightfieldDataFloat;
		const double* m_heightfieldDataDouble;
		const void* m_heightfieldDataUnknown;
	};

	PHY_ScalarType m_heightDataType;
	bool m_flipQuadEdges;
	bool m_useDiamondSubdivision;
	bool m_useZigzagSubdivision;
	bool m_flipTriangleWinding;
	int m_upAxis;

	btVector3 m_localScaling;

	
	btAlignedObjectArray<Range> m_vboundsGrid;
	int m_vboundsGridWidth;
	int m_vboundsGridLength;
	int m_vboundsChunkSize;

	
	btScalar m_userValue3;

	struct btTriangleInfoMap* m_triangleInfoMap;

	virtual btScalar getRawHeightFieldValue(int x, int y) const;
	void quantizeWithClamp(int* out, const btVector3& point, int isMax) const;

	
	
	void initialize(int heightStickWidth, int heightStickLength,
					const void* heightfieldData, btScalar heightScale,
					btScalar minHeight, btScalar maxHeight, int upAxis,
					PHY_ScalarType heightDataType, bool flipQuadEdges);

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	
	btHeightfieldTerrainShape(
		int heightStickWidth, int heightStickLength,
		const float* heightfieldData, btScalar minHeight, btScalar maxHeight,
		int upAxis, bool flipQuadEdges);
	btHeightfieldTerrainShape(
		int heightStickWidth, int heightStickLength,
		const double* heightfieldData, btScalar minHeight, btScalar maxHeight,
		int upAxis, bool flipQuadEdges);
	btHeightfieldTerrainShape(
		int heightStickWidth, int heightStickLength,
		const short* heightfieldData, btScalar heightScale, btScalar minHeight, btScalar maxHeight,
		int upAxis, bool flipQuadEdges);
	btHeightfieldTerrainShape(
		int heightStickWidth, int heightStickLength,
		const unsigned char* heightfieldData, btScalar heightScale, btScalar minHeight, btScalar maxHeight,
		int upAxis, bool flipQuadEdges);

	
	
	btHeightfieldTerrainShape(int heightStickWidth, int heightStickLength,
							  const void* heightfieldData, btScalar heightScale,
							  btScalar minHeight, btScalar maxHeight,
							  int upAxis, PHY_ScalarType heightDataType,
							  bool flipQuadEdges);

	
	
	btHeightfieldTerrainShape(int heightStickWidth, int heightStickLength, const void* heightfieldData, btScalar maxHeight, int upAxis, bool useFloatData, bool flipQuadEdges);

	virtual ~btHeightfieldTerrainShape();

	void setUseDiamondSubdivision(bool useDiamondSubdivision = true) { m_useDiamondSubdivision = useDiamondSubdivision; }

	
	void setUseZigzagSubdivision(bool useZigzagSubdivision = true) { m_useZigzagSubdivision = useZigzagSubdivision; }

	void setFlipTriangleWinding(bool flipTriangleWinding)
	{
		m_flipTriangleWinding = flipTriangleWinding;
	}
	virtual void getAabb(const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const;

	virtual void processAllTriangles(btTriangleCallback * callback, const btVector3& aabbMin, const btVector3& aabbMax) const;

	virtual void calculateLocalInertia(btScalar mass, btVector3 & inertia) const;

	virtual void setLocalScaling(const btVector3& scaling);

	virtual const btVector3& getLocalScaling() const;

	void getVertex(int x, int y, btVector3& vertex) const;

	void performRaycast(btTriangleCallback * callback, const btVector3& raySource, const btVector3& rayTarget) const;

	void buildAccelerator(int chunkSize = 16);
	void clearAccelerator();

	int getUpAxis() const
	{
		return m_upAxis;
	}
	
	virtual const char* getName() const { return "HEIGHTFIELD"; }

	
	void setUserValue3(btScalar value)
	{
		m_userValue3 = value;
	}
	btScalar getUserValue3() const
	{
		return m_userValue3;
	}
	const struct btTriangleInfoMap* getTriangleInfoMap() const
	{
		return m_triangleInfoMap;
	}
	struct btTriangleInfoMap* getTriangleInfoMap()
	{
		return m_triangleInfoMap;
	}
	void setTriangleInfoMap(btTriangleInfoMap* map)
	{
		m_triangleInfoMap = map;
	}
	const unsigned char* getHeightfieldRawData() const
	{
		return m_heightfieldDataUnsignedChar;
	}
};

#endif  
