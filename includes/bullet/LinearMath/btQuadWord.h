

#ifndef BT_SIMD_QUADWORD_H
#define BT_SIMD_QUADWORD_H

#include "btScalar.h"
#include "btMinMax.h"

#if defined(__CELLOS_LV2) && defined(__SPU__)
#include <altivec.h>
#endif


#ifndef USE_LIBSPE2
ATTRIBUTE_ALIGNED16(class)
btQuadWord
#else
class btQuadWord
#endif
{
protected:
#if defined(__SPU__) && defined(__CELLOS_LV2__)
	union {
		vec_float4 mVec128;
		btScalar m_floats[4];
	};

public:
	vec_float4 get128() const
	{
		return mVec128;
	}

protected:
#else  

#if defined(BT_USE_SSE) || defined(BT_USE_NEON)
	union {
		btSimdFloat4 mVec128;
		btScalar m_floats[4];
	};

public:
	SIMD_FORCE_INLINE btSimdFloat4 get128() const
	{
		return mVec128;
	}
	SIMD_FORCE_INLINE void set128(btSimdFloat4 v128)
	{
		mVec128 = v128;
	}
#else
	btScalar m_floats[4];
#endif  

#endif  

public:
#if (defined(BT_USE_SSE_IN_API) && defined(BT_USE_SSE)) || defined(BT_USE_NEON)

	
	SIMD_FORCE_INLINE btQuadWord(const btSimdFloat4 vec)
	{
		mVec128 = vec;
	}

	
	SIMD_FORCE_INLINE btQuadWord(const btQuadWord& rhs)
	{
		mVec128 = rhs.mVec128;
	}

	
	SIMD_FORCE_INLINE btQuadWord&
	operator=(const btQuadWord& v)
	{
		mVec128 = v.mVec128;

		return *this;
	}

#endif

	
	SIMD_FORCE_INLINE const btScalar& getX() const { return m_floats[0]; }
	
	SIMD_FORCE_INLINE const btScalar& getY() const { return m_floats[1]; }
	
	SIMD_FORCE_INLINE const btScalar& getZ() const { return m_floats[2]; }
	
	SIMD_FORCE_INLINE void setX(btScalar _x) { m_floats[0] = _x; };
	
	SIMD_FORCE_INLINE void setY(btScalar _y) { m_floats[1] = _y; };
	
	SIMD_FORCE_INLINE void setZ(btScalar _z) { m_floats[2] = _z; };
	
	SIMD_FORCE_INLINE void setW(btScalar _w) { m_floats[3] = _w; };
	
	SIMD_FORCE_INLINE const btScalar& x() const { return m_floats[0]; }
	
	SIMD_FORCE_INLINE const btScalar& y() const { return m_floats[1]; }
	
	SIMD_FORCE_INLINE const btScalar& z() const { return m_floats[2]; }
	
	SIMD_FORCE_INLINE const btScalar& w() const { return m_floats[3]; }

	
	
	
	SIMD_FORCE_INLINE operator btScalar*() { return &m_floats[0]; }
	SIMD_FORCE_INLINE operator const btScalar*() const { return &m_floats[0]; }

	SIMD_FORCE_INLINE bool operator==(const btQuadWord& other) const
	{
#ifdef BT_USE_SSE
		return (0xf == _mm_movemask_ps((__m128)_mm_cmpeq_ps(mVec128, other.mVec128)));
#else
		return ((m_floats[3] == other.m_floats[3]) &&
				(m_floats[2] == other.m_floats[2]) &&
				(m_floats[1] == other.m_floats[1]) &&
				(m_floats[0] == other.m_floats[0]));
#endif
	}

	SIMD_FORCE_INLINE bool operator!=(const btQuadWord& other) const
	{
		return !(*this == other);
	}

	
	SIMD_FORCE_INLINE void setValue(const btScalar& _x, const btScalar& _y, const btScalar& _z)
	{
		m_floats[0] = _x;
		m_floats[1] = _y;
		m_floats[2] = _z;
		m_floats[3] = 0.f;
	}

	
	
	SIMD_FORCE_INLINE void setValue(const btScalar& _x, const btScalar& _y, const btScalar& _z, const btScalar& _w)
	{
		m_floats[0] = _x;
		m_floats[1] = _y;
		m_floats[2] = _z;
		m_floats[3] = _w;
	}
	
	SIMD_FORCE_INLINE btQuadWord()
	
	{
	}

	
	SIMD_FORCE_INLINE btQuadWord(const btScalar& _x, const btScalar& _y, const btScalar& _z)
	{
		m_floats[0] = _x, m_floats[1] = _y, m_floats[2] = _z, m_floats[3] = 0.0f;
	}

	
	SIMD_FORCE_INLINE btQuadWord(const btScalar& _x, const btScalar& _y, const btScalar& _z, const btScalar& _w)
	{
		m_floats[0] = _x, m_floats[1] = _y, m_floats[2] = _z, m_floats[3] = _w;
	}

	
	SIMD_FORCE_INLINE void setMax(const btQuadWord& other)
	{
#ifdef BT_USE_SSE
		mVec128 = _mm_max_ps(mVec128, other.mVec128);
#elif defined(BT_USE_NEON)
		mVec128 = vmaxq_f32(mVec128, other.mVec128);
#else
		btSetMax(m_floats[0], other.m_floats[0]);
		btSetMax(m_floats[1], other.m_floats[1]);
		btSetMax(m_floats[2], other.m_floats[2]);
		btSetMax(m_floats[3], other.m_floats[3]);
#endif
	}
	
	SIMD_FORCE_INLINE void setMin(const btQuadWord& other)
	{
#ifdef BT_USE_SSE
		mVec128 = _mm_min_ps(mVec128, other.mVec128);
#elif defined(BT_USE_NEON)
		mVec128 = vminq_f32(mVec128, other.mVec128);
#else
		btSetMin(m_floats[0], other.m_floats[0]);
		btSetMin(m_floats[1], other.m_floats[1]);
		btSetMin(m_floats[2], other.m_floats[2]);
		btSetMin(m_floats[3], other.m_floats[3]);
#endif
	}
};

#endif  
