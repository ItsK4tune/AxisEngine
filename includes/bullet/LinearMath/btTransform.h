

#ifndef BT_TRANSFORM_H
#define BT_TRANSFORM_H

#include "btMatrix3x3.h"

#ifdef BT_USE_DOUBLE_PRECISION
#define btTransformData btTransformDoubleData
#else
#define btTransformData btTransformFloatData
#endif


ATTRIBUTE_ALIGNED16(class)
btTransform
{
	
	btMatrix3x3 m_basis;
	
	btVector3 m_origin;

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();
	
	btTransform() {}
	
	explicit SIMD_FORCE_INLINE btTransform(const btQuaternion& q,
										   const btVector3& c = btVector3(btScalar(0), btScalar(0), btScalar(0)))
		: m_basis(q),
		  m_origin(c)
	{
	}

	
	explicit SIMD_FORCE_INLINE btTransform(const btMatrix3x3& b,
										   const btVector3& c = btVector3(btScalar(0), btScalar(0), btScalar(0)))
		: m_basis(b),
		  m_origin(c)
	{
	}
	
	SIMD_FORCE_INLINE btTransform(const btTransform& other)
		: m_basis(other.m_basis),
		  m_origin(other.m_origin)
	{
	}
	
	SIMD_FORCE_INLINE btTransform& operator=(const btTransform& other)
	{
		m_basis = other.m_basis;
		m_origin = other.m_origin;
		return *this;
	}

	
	SIMD_FORCE_INLINE void mult(const btTransform& t1, const btTransform& t2)
	{
		m_basis = t1.m_basis * t2.m_basis;
		m_origin = t1(t2.m_origin);
	}

	

	
	SIMD_FORCE_INLINE btVector3 operator()(const btVector3& x) const
	{
		return x.dot3(m_basis[0], m_basis[1], m_basis[2]) + m_origin;
	}

	
	SIMD_FORCE_INLINE btVector3 operator*(const btVector3& x) const
	{
		return (*this)(x);
	}

	
	SIMD_FORCE_INLINE btQuaternion operator*(const btQuaternion& q) const
	{
		return getRotation() * q;
	}

	
	SIMD_FORCE_INLINE btMatrix3x3& getBasis() { return m_basis; }
	
	SIMD_FORCE_INLINE const btMatrix3x3& getBasis() const { return m_basis; }

	
	SIMD_FORCE_INLINE btVector3& getOrigin() { return m_origin; }
	
	SIMD_FORCE_INLINE const btVector3& getOrigin() const { return m_origin; }

	
	btQuaternion getRotation() const
	{
		btQuaternion q;
		m_basis.getRotation(q);
		return q;
	}

	
	void setFromOpenGLMatrix(const btScalar* m)
	{
		m_basis.setFromOpenGLSubMatrix(m);
		m_origin.setValue(m[12], m[13], m[14]);
	}

	
	void getOpenGLMatrix(btScalar * m) const
	{
		m_basis.getOpenGLSubMatrix(m);
		m[12] = m_origin.x();
		m[13] = m_origin.y();
		m[14] = m_origin.z();
		m[15] = btScalar(1.0);
	}

	
	SIMD_FORCE_INLINE void setOrigin(const btVector3& origin)
	{
		m_origin = origin;
	}

	SIMD_FORCE_INLINE btVector3 invXform(const btVector3& inVec) const;

	
	SIMD_FORCE_INLINE void setBasis(const btMatrix3x3& basis)
	{
		m_basis = basis;
	}

	
	SIMD_FORCE_INLINE void setRotation(const btQuaternion& q)
	{
		m_basis.setRotation(q);
	}

	
	void setIdentity()
	{
		m_basis.setIdentity();
		m_origin.setValue(btScalar(0.0), btScalar(0.0), btScalar(0.0));
	}

	
	btTransform& operator*=(const btTransform& t)
	{
		m_origin += m_basis * t.m_origin;
		m_basis *= t.m_basis;
		return *this;
	}

	
	btTransform inverse() const
	{
		btMatrix3x3 inv = m_basis.transpose();
		return btTransform(inv, inv * -m_origin);
	}

	
	btTransform inverseTimes(const btTransform& t) const;

	
	btTransform operator*(const btTransform& t) const;

	
	static const btTransform& getIdentity()
	{
		static const btTransform identityTransform(btMatrix3x3::getIdentity());
		return identityTransform;
	}

	void serialize(struct btTransformData & dataOut) const;

	void serializeFloat(struct btTransformFloatData & dataOut) const;

	void deSerialize(const struct btTransformData& dataIn);

	void deSerializeDouble(const struct btTransformDoubleData& dataIn);

	void deSerializeFloat(const struct btTransformFloatData& dataIn);
};

SIMD_FORCE_INLINE btVector3
btTransform::invXform(const btVector3& inVec) const
{
	btVector3 v = inVec - m_origin;
	return (m_basis.transpose() * v);
}

SIMD_FORCE_INLINE btTransform
btTransform::inverseTimes(const btTransform& t) const
{
	btVector3 v = t.getOrigin() - m_origin;
	return btTransform(m_basis.transposeTimes(t.m_basis),
					   v * m_basis);
}

SIMD_FORCE_INLINE btTransform
	btTransform::operator*(const btTransform& t) const
{
	return btTransform(m_basis * t.m_basis,
					   (*this)(t.m_origin));
}


SIMD_FORCE_INLINE bool operator==(const btTransform& t1, const btTransform& t2)
{
	return (t1.getBasis() == t2.getBasis() &&
			t1.getOrigin() == t2.getOrigin());
}


struct btTransformFloatData
{
	btMatrix3x3FloatData m_basis;
	btVector3FloatData m_origin;
};

struct btTransformDoubleData
{
	btMatrix3x3DoubleData m_basis;
	btVector3DoubleData m_origin;
};

SIMD_FORCE_INLINE void btTransform::serialize(btTransformData& dataOut) const
{
	m_basis.serialize(dataOut.m_basis);
	m_origin.serialize(dataOut.m_origin);
}

SIMD_FORCE_INLINE void btTransform::serializeFloat(btTransformFloatData& dataOut) const
{
	m_basis.serializeFloat(dataOut.m_basis);
	m_origin.serializeFloat(dataOut.m_origin);
}

SIMD_FORCE_INLINE void btTransform::deSerialize(const btTransformData& dataIn)
{
	m_basis.deSerialize(dataIn.m_basis);
	m_origin.deSerialize(dataIn.m_origin);
}

SIMD_FORCE_INLINE void btTransform::deSerializeFloat(const btTransformFloatData& dataIn)
{
	m_basis.deSerializeFloat(dataIn.m_basis);
	m_origin.deSerializeFloat(dataIn.m_origin);
}

SIMD_FORCE_INLINE void btTransform::deSerializeDouble(const btTransformDoubleData& dataIn)
{
	m_basis.deSerializeDouble(dataIn.m_basis);
	m_origin.deSerializeDouble(dataIn.m_origin);
}

#endif  
