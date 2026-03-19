

#ifndef BT_SIMD__QUATERNION_H_
#define BT_SIMD__QUATERNION_H_

#include "btVector3.h"
#include "btQuadWord.h"

#ifdef BT_USE_DOUBLE_PRECISION
#define btQuaternionData btQuaternionDoubleData
#define btQuaternionDataName "btQuaternionDoubleData"
#else
#define btQuaternionData btQuaternionFloatData
#define btQuaternionDataName "btQuaternionFloatData"
#endif  

#ifdef BT_USE_SSE


#define vOnes (_mm_set_ps(1.0f, 1.0f, 1.0f, 1.0f))

#endif

#if defined(BT_USE_SSE)

#define vQInv (_mm_set_ps(+0.0f, -0.0f, -0.0f, -0.0f))
#define vPPPM (_mm_set_ps(-0.0f, +0.0f, +0.0f, +0.0f))

#elif defined(BT_USE_NEON)

const btSimdFloat4 ATTRIBUTE_ALIGNED16(vQInv) = {-0.0f, -0.0f, -0.0f, +0.0f};
const btSimdFloat4 ATTRIBUTE_ALIGNED16(vPPPM) = {+0.0f, +0.0f, +0.0f, -0.0f};

#endif


class btQuaternion : public btQuadWord
{
public:
	
	btQuaternion() {}

#if (defined(BT_USE_SSE_IN_API) && defined(BT_USE_SSE)) || defined(BT_USE_NEON)
	
	SIMD_FORCE_INLINE btQuaternion(const btSimdFloat4 vec)
	{
		mVec128 = vec;
	}

	
	SIMD_FORCE_INLINE btQuaternion(const btQuaternion& rhs)
	{
		mVec128 = rhs.mVec128;
	}

	
	SIMD_FORCE_INLINE btQuaternion&
	operator=(const btQuaternion& v)
	{
		mVec128 = v.mVec128;

		return *this;
	}

#endif

	
	
	
	btQuaternion(const btScalar& _x, const btScalar& _y, const btScalar& _z, const btScalar& _w)
		: btQuadWord(_x, _y, _z, _w)
	{
	}
	
	btQuaternion(const btVector3& _axis, const btScalar& _angle)
	{
		setRotation(_axis, _angle);
	}
	
	btQuaternion(const btScalar& yaw, const btScalar& pitch, const btScalar& roll)
	{
#ifndef BT_EULER_DEFAULT_ZYX
		setEuler(yaw, pitch, roll);
#else
		setEulerZYX(yaw, pitch, roll);
#endif
	}
	
	void setRotation(const btVector3& axis, const btScalar& _angle)
	{
		btScalar d = axis.length();
		btAssert(d != btScalar(0.0));
		btScalar s = btSin(_angle * btScalar(0.5)) / d;
		setValue(axis.x() * s, axis.y() * s, axis.z() * s,
				 btCos(_angle * btScalar(0.5)));
	}
	
	void setEuler(const btScalar& yaw, const btScalar& pitch, const btScalar& roll)
	{
		btScalar halfYaw = btScalar(yaw) * btScalar(0.5);
		btScalar halfPitch = btScalar(pitch) * btScalar(0.5);
		btScalar halfRoll = btScalar(roll) * btScalar(0.5);
		btScalar cosYaw = btCos(halfYaw);
		btScalar sinYaw = btSin(halfYaw);
		btScalar cosPitch = btCos(halfPitch);
		btScalar sinPitch = btSin(halfPitch);
		btScalar cosRoll = btCos(halfRoll);
		btScalar sinRoll = btSin(halfRoll);
		setValue(cosRoll * sinPitch * cosYaw + sinRoll * cosPitch * sinYaw,
				 cosRoll * cosPitch * sinYaw - sinRoll * sinPitch * cosYaw,
				 sinRoll * cosPitch * cosYaw - cosRoll * sinPitch * sinYaw,
				 cosRoll * cosPitch * cosYaw + sinRoll * sinPitch * sinYaw);
	}
	
	void setEulerZYX(const btScalar& yawZ, const btScalar& pitchY, const btScalar& rollX)
	{
		btScalar halfYaw = btScalar(yawZ) * btScalar(0.5);
		btScalar halfPitch = btScalar(pitchY) * btScalar(0.5);
		btScalar halfRoll = btScalar(rollX) * btScalar(0.5);
		btScalar cosYaw = btCos(halfYaw);
		btScalar sinYaw = btSin(halfYaw);
		btScalar cosPitch = btCos(halfPitch);
		btScalar sinPitch = btSin(halfPitch);
		btScalar cosRoll = btCos(halfRoll);
		btScalar sinRoll = btSin(halfRoll);
		setValue(sinRoll * cosPitch * cosYaw - cosRoll * sinPitch * sinYaw,   
				 cosRoll * sinPitch * cosYaw + sinRoll * cosPitch * sinYaw,   
				 cosRoll * cosPitch * sinYaw - sinRoll * sinPitch * cosYaw,   
				 cosRoll * cosPitch * cosYaw + sinRoll * sinPitch * sinYaw);  
	}

	
	void getEulerZYX(btScalar& yawZ, btScalar& pitchY, btScalar& rollX) const
	{
		btScalar squ;
		btScalar sqx;
		btScalar sqy;
		btScalar sqz;
		btScalar sarg;
		sqx = m_floats[0] * m_floats[0];
		sqy = m_floats[1] * m_floats[1];
		sqz = m_floats[2] * m_floats[2];
		squ = m_floats[3] * m_floats[3];
		sarg = btScalar(-2.) * (m_floats[0] * m_floats[2] - m_floats[3] * m_floats[1]);

		
		
		
		
		if (sarg <= -btScalar(0.99999))
		{
			pitchY = btScalar(-0.5) * SIMD_PI;
			rollX = 0;
			yawZ = btScalar(2) * btAtan2(m_floats[0], -m_floats[1]);
		}
		else if (sarg >= btScalar(0.99999))
		{
			pitchY = btScalar(0.5) * SIMD_PI;
			rollX = 0;
			yawZ = btScalar(2) * btAtan2(-m_floats[0], m_floats[1]);
		}
		else
		{
			pitchY = btAsin(sarg);
			rollX = btAtan2(2 * (m_floats[1] * m_floats[2] + m_floats[3] * m_floats[0]), squ - sqx - sqy + sqz);
			yawZ = btAtan2(2 * (m_floats[0] * m_floats[1] + m_floats[3] * m_floats[2]), squ + sqx - sqy - sqz);
		}
	}

	
	SIMD_FORCE_INLINE btQuaternion& operator+=(const btQuaternion& q)
	{
#if defined(BT_USE_SSE_IN_API) && defined(BT_USE_SSE)
		mVec128 = _mm_add_ps(mVec128, q.mVec128);
#elif defined(BT_USE_NEON)
		mVec128 = vaddq_f32(mVec128, q.mVec128);
#else
		m_floats[0] += q.x();
		m_floats[1] += q.y();
		m_floats[2] += q.z();
		m_floats[3] += q.m_floats[3];
#endif
		return *this;
	}

	
	btQuaternion& operator-=(const btQuaternion& q)
	{
#if defined(BT_USE_SSE_IN_API) && defined(BT_USE_SSE)
		mVec128 = _mm_sub_ps(mVec128, q.mVec128);
#elif defined(BT_USE_NEON)
		mVec128 = vsubq_f32(mVec128, q.mVec128);
#else
		m_floats[0] -= q.x();
		m_floats[1] -= q.y();
		m_floats[2] -= q.z();
		m_floats[3] -= q.m_floats[3];
#endif
		return *this;
	}

	
	btQuaternion& operator*=(const btScalar& s)
	{
#if defined(BT_USE_SSE_IN_API) && defined(BT_USE_SSE)
		__m128 vs = _mm_load_ss(&s);  
		vs = bt_pshufd_ps(vs, 0);     
		mVec128 = _mm_mul_ps(mVec128, vs);
#elif defined(BT_USE_NEON)
		mVec128 = vmulq_n_f32(mVec128, s);
#else
		m_floats[0] *= s;
		m_floats[1] *= s;
		m_floats[2] *= s;
		m_floats[3] *= s;
#endif
		return *this;
	}

	
	btQuaternion& operator*=(const btQuaternion& q)
	{
#if defined(BT_USE_SSE_IN_API) && defined(BT_USE_SSE)
		__m128 vQ2 = q.get128();

		__m128 A1 = bt_pshufd_ps(mVec128, BT_SHUFFLE(0, 1, 2, 0));
		__m128 B1 = bt_pshufd_ps(vQ2, BT_SHUFFLE(3, 3, 3, 0));

		A1 = A1 * B1;

		__m128 A2 = bt_pshufd_ps(mVec128, BT_SHUFFLE(1, 2, 0, 1));
		__m128 B2 = bt_pshufd_ps(vQ2, BT_SHUFFLE(2, 0, 1, 1));

		A2 = A2 * B2;

		B1 = bt_pshufd_ps(mVec128, BT_SHUFFLE(2, 0, 1, 2));
		B2 = bt_pshufd_ps(vQ2, BT_SHUFFLE(1, 2, 0, 2));

		B1 = B1 * B2;  

		mVec128 = bt_splat_ps(mVec128, 3);  
		mVec128 = mVec128 * vQ2;            

		A1 = A1 + A2;                
		mVec128 = mVec128 - B1;      
		A1 = _mm_xor_ps(A1, vPPPM);  
		mVec128 = mVec128 + A1;      

#elif defined(BT_USE_NEON)

		float32x4_t vQ1 = mVec128;
		float32x4_t vQ2 = q.get128();
		float32x4_t A0, A1, B1, A2, B2, A3, B3;
		float32x2_t vQ1zx, vQ2wx, vQ1yz, vQ2zx, vQ2yz, vQ2xz;

		{
			float32x2x2_t tmp;
			tmp = vtrn_f32(vget_high_f32(vQ1), vget_low_f32(vQ1));  
			vQ1zx = tmp.val[0];

			tmp = vtrn_f32(vget_high_f32(vQ2), vget_low_f32(vQ2));  
			vQ2zx = tmp.val[0];
		}
		vQ2wx = vext_f32(vget_high_f32(vQ2), vget_low_f32(vQ2), 1);

		vQ1yz = vext_f32(vget_low_f32(vQ1), vget_high_f32(vQ1), 1);

		vQ2yz = vext_f32(vget_low_f32(vQ2), vget_high_f32(vQ2), 1);
		vQ2xz = vext_f32(vQ2zx, vQ2zx, 1);

		A1 = vcombine_f32(vget_low_f32(vQ1), vQ1zx);                     
		B1 = vcombine_f32(vdup_lane_f32(vget_high_f32(vQ2), 1), vQ2wx);  

		A2 = vcombine_f32(vQ1yz, vget_low_f32(vQ1));
		B2 = vcombine_f32(vQ2zx, vdup_lane_f32(vget_low_f32(vQ2), 1));

		A3 = vcombine_f32(vQ1zx, vQ1yz);  
		B3 = vcombine_f32(vQ2yz, vQ2xz);  

		A1 = vmulq_f32(A1, B1);
		A2 = vmulq_f32(A2, B2);
		A3 = vmulq_f32(A3, B3);                           
		A0 = vmulq_lane_f32(vQ2, vget_high_f32(vQ1), 1);  

		A1 = vaddq_f32(A1, A2);  
		A0 = vsubq_f32(A0, A3);  

		
		A1 = (btSimdFloat4)veorq_s32((int32x4_t)A1, (int32x4_t)vPPPM);
		A0 = vaddq_f32(A0, A1);  

		mVec128 = A0;
#else
		setValue(
			m_floats[3] * q.x() + m_floats[0] * q.m_floats[3] + m_floats[1] * q.z() - m_floats[2] * q.y(),
			m_floats[3] * q.y() + m_floats[1] * q.m_floats[3] + m_floats[2] * q.x() - m_floats[0] * q.z(),
			m_floats[3] * q.z() + m_floats[2] * q.m_floats[3] + m_floats[0] * q.y() - m_floats[1] * q.x(),
			m_floats[3] * q.m_floats[3] - m_floats[0] * q.x() - m_floats[1] * q.y() - m_floats[2] * q.z());
#endif
		return *this;
	}
	
	btScalar dot(const btQuaternion& q) const
	{
#if defined BT_USE_SIMD_VECTOR3 && defined(BT_USE_SSE_IN_API) && defined(BT_USE_SSE)
		__m128 vd;

		vd = _mm_mul_ps(mVec128, q.mVec128);

		__m128 t = _mm_movehl_ps(vd, vd);
		vd = _mm_add_ps(vd, t);
		t = _mm_shuffle_ps(vd, vd, 0x55);
		vd = _mm_add_ss(vd, t);

		return _mm_cvtss_f32(vd);
#elif defined(BT_USE_NEON)
		float32x4_t vd = vmulq_f32(mVec128, q.mVec128);
		float32x2_t x = vpadd_f32(vget_low_f32(vd), vget_high_f32(vd));
		x = vpadd_f32(x, x);
		return vget_lane_f32(x, 0);
#else
		return m_floats[0] * q.x() +
			   m_floats[1] * q.y() +
			   m_floats[2] * q.z() +
			   m_floats[3] * q.m_floats[3];
#endif
	}

	
	btScalar length2() const
	{
		return dot(*this);
	}

	
	btScalar length() const
	{
		return btSqrt(length2());
	}
	btQuaternion& safeNormalize()
	{
		btScalar l2 = length2();
		if (l2 > SIMD_EPSILON)
		{
			normalize();
		}
		return *this;
	}
	
	btQuaternion& normalize()
	{
#if defined(BT_USE_SSE_IN_API) && defined(BT_USE_SSE)
		__m128 vd;

		vd = _mm_mul_ps(mVec128, mVec128);

		__m128 t = _mm_movehl_ps(vd, vd);
		vd = _mm_add_ps(vd, t);
		t = _mm_shuffle_ps(vd, vd, 0x55);
		vd = _mm_add_ss(vd, t);

		vd = _mm_sqrt_ss(vd);
		vd = _mm_div_ss(vOnes, vd);
		vd = bt_pshufd_ps(vd, 0);  
		mVec128 = _mm_mul_ps(mVec128, vd);

		return *this;
#else
		return *this /= length();
#endif
	}

	
	SIMD_FORCE_INLINE btQuaternion
	operator*(const btScalar& s) const
	{
#if defined(BT_USE_SSE_IN_API) && defined(BT_USE_SSE)
		__m128 vs = _mm_load_ss(&s);  
		vs = bt_pshufd_ps(vs, 0x00);  

		return btQuaternion(_mm_mul_ps(mVec128, vs));
#elif defined(BT_USE_NEON)
		return btQuaternion(vmulq_n_f32(mVec128, s));
#else
		return btQuaternion(x() * s, y() * s, z() * s, m_floats[3] * s);
#endif
	}

	
	btQuaternion operator/(const btScalar& s) const
	{
		btAssert(s != btScalar(0.0));
		return *this * (btScalar(1.0) / s);
	}

	
	btQuaternion& operator/=(const btScalar& s)
	{
		btAssert(s != btScalar(0.0));
		return *this *= btScalar(1.0) / s;
	}

	
	btQuaternion normalized() const
	{
		return *this / length();
	}
	
	btScalar angle(const btQuaternion& q) const
	{
		btScalar s = btSqrt(length2() * q.length2());
		btAssert(s != btScalar(0.0));
		return btAcos(dot(q) / s);
	}

	
	btScalar angleShortestPath(const btQuaternion& q) const
	{
		btScalar s = btSqrt(length2() * q.length2());
		btAssert(s != btScalar(0.0));
		if (dot(q) < 0)  
			return btAcos(dot(-q) / s) * btScalar(2.0);
		else
			return btAcos(dot(q) / s) * btScalar(2.0);
	}

	
	btScalar getAngle() const
	{
		btScalar s = btScalar(2.) * btAcos(m_floats[3]);
		return s;
	}

	
	btScalar getAngleShortestPath() const
	{
		btScalar s;
		if (m_floats[3] >= 0)
			s = btScalar(2.) * btAcos(m_floats[3]);
		else
			s = btScalar(2.) * btAcos(-m_floats[3]);
		return s;
	}

	
	btVector3 getAxis() const
	{
		btScalar s_squared = 1.f - m_floats[3] * m_floats[3];

		if (s_squared < btScalar(10.) * SIMD_EPSILON)  
			return btVector3(1.0, 0.0, 0.0);           
		btScalar s = 1.f / btSqrt(s_squared);
		return btVector3(m_floats[0] * s, m_floats[1] * s, m_floats[2] * s);
	}

	
	btQuaternion inverse() const
	{
#if defined(BT_USE_SSE_IN_API) && defined(BT_USE_SSE)
		return btQuaternion(_mm_xor_ps(mVec128, vQInv));
#elif defined(BT_USE_NEON)
		return btQuaternion((btSimdFloat4)veorq_s32((int32x4_t)mVec128, (int32x4_t)vQInv));
#else
		return btQuaternion(-m_floats[0], -m_floats[1], -m_floats[2], m_floats[3]);
#endif
	}

	
	SIMD_FORCE_INLINE btQuaternion
	operator+(const btQuaternion& q2) const
	{
#if defined(BT_USE_SSE_IN_API) && defined(BT_USE_SSE)
		return btQuaternion(_mm_add_ps(mVec128, q2.mVec128));
#elif defined(BT_USE_NEON)
		return btQuaternion(vaddq_f32(mVec128, q2.mVec128));
#else
		const btQuaternion& q1 = *this;
		return btQuaternion(q1.x() + q2.x(), q1.y() + q2.y(), q1.z() + q2.z(), q1.m_floats[3] + q2.m_floats[3]);
#endif
	}

	
	SIMD_FORCE_INLINE btQuaternion
	operator-(const btQuaternion& q2) const
	{
#if defined(BT_USE_SSE_IN_API) && defined(BT_USE_SSE)
		return btQuaternion(_mm_sub_ps(mVec128, q2.mVec128));
#elif defined(BT_USE_NEON)
		return btQuaternion(vsubq_f32(mVec128, q2.mVec128));
#else
		const btQuaternion& q1 = *this;
		return btQuaternion(q1.x() - q2.x(), q1.y() - q2.y(), q1.z() - q2.z(), q1.m_floats[3] - q2.m_floats[3]);
#endif
	}

	
	SIMD_FORCE_INLINE btQuaternion operator-() const
	{
#if defined(BT_USE_SSE_IN_API) && defined(BT_USE_SSE)
		return btQuaternion(_mm_xor_ps(mVec128, btvMzeroMask));
#elif defined(BT_USE_NEON)
		return btQuaternion((btSimdFloat4)veorq_s32((int32x4_t)mVec128, (int32x4_t)btvMzeroMask));
#else
		const btQuaternion& q2 = *this;
		return btQuaternion(-q2.x(), -q2.y(), -q2.z(), -q2.m_floats[3]);
#endif
	}
	
	SIMD_FORCE_INLINE btQuaternion farthest(const btQuaternion& qd) const
	{
		btQuaternion diff, sum;
		diff = *this - qd;
		sum = *this + qd;
		if (diff.dot(diff) > sum.dot(sum))
			return qd;
		return (-qd);
	}

	
	SIMD_FORCE_INLINE btQuaternion nearest(const btQuaternion& qd) const
	{
		btQuaternion diff, sum;
		diff = *this - qd;
		sum = *this + qd;
		if (diff.dot(diff) < sum.dot(sum))
			return qd;
		return (-qd);
	}

	
	btQuaternion slerp(const btQuaternion& q, const btScalar& t) const
	{
		const btScalar magnitude = btSqrt(length2() * q.length2());
		btAssert(magnitude > btScalar(0));

		const btScalar product = dot(q) / magnitude;
		const btScalar absproduct = btFabs(product);

		if (absproduct < btScalar(1.0 - SIMD_EPSILON))
		{
			
			const btScalar theta = btAcos(absproduct);
			const btScalar d = btSin(theta);
			btAssert(d > btScalar(0));

			const btScalar sign = (product < 0) ? btScalar(-1) : btScalar(1);
			const btScalar s0 = btSin((btScalar(1.0) - t) * theta) / d;
			const btScalar s1 = btSin(sign * t * theta) / d;

			return btQuaternion(
				(m_floats[0] * s0 + q.x() * s1),
				(m_floats[1] * s0 + q.y() * s1),
				(m_floats[2] * s0 + q.z() * s1),
				(m_floats[3] * s0 + q.w() * s1));
		}
		else
		{
			return *this;
		}
	}

	static const btQuaternion& getIdentity()
	{
		static const btQuaternion identityQuat(btScalar(0.), btScalar(0.), btScalar(0.), btScalar(1.));
		return identityQuat;
	}

	SIMD_FORCE_INLINE const btScalar& getW() const { return m_floats[3]; }

	SIMD_FORCE_INLINE void serialize(struct btQuaternionData& dataOut) const;

	SIMD_FORCE_INLINE void deSerialize(const struct btQuaternionFloatData& dataIn);

	SIMD_FORCE_INLINE void deSerialize(const struct btQuaternionDoubleData& dataIn);

	SIMD_FORCE_INLINE void serializeFloat(struct btQuaternionFloatData& dataOut) const;

	SIMD_FORCE_INLINE void deSerializeFloat(const struct btQuaternionFloatData& dataIn);

	SIMD_FORCE_INLINE void serializeDouble(struct btQuaternionDoubleData& dataOut) const;

	SIMD_FORCE_INLINE void deSerializeDouble(const struct btQuaternionDoubleData& dataIn);
};


SIMD_FORCE_INLINE btQuaternion
operator*(const btQuaternion& q1, const btQuaternion& q2)
{
#if defined(BT_USE_SSE_IN_API) && defined(BT_USE_SSE)
	__m128 vQ1 = q1.get128();
	__m128 vQ2 = q2.get128();
	__m128 A0, A1, B1, A2, B2;

	A1 = bt_pshufd_ps(vQ1, BT_SHUFFLE(0, 1, 2, 0));  
	B1 = bt_pshufd_ps(vQ2, BT_SHUFFLE(3, 3, 3, 0));  

	A1 = A1 * B1;

	A2 = bt_pshufd_ps(vQ1, BT_SHUFFLE(1, 2, 0, 1));  
	B2 = bt_pshufd_ps(vQ2, BT_SHUFFLE(2, 0, 1, 1));  

	A2 = A2 * B2;

	B1 = bt_pshufd_ps(vQ1, BT_SHUFFLE(2, 0, 1, 2));  
	B2 = bt_pshufd_ps(vQ2, BT_SHUFFLE(1, 2, 0, 2));  

	B1 = B1 * B2;  

	A0 = bt_splat_ps(vQ1, 3);  
	A0 = A0 * vQ2;             

	A1 = A1 + A2;  
	A0 = A0 - B1;  

	A1 = _mm_xor_ps(A1, vPPPM);  
	A0 = A0 + A1;                

	return btQuaternion(A0);

#elif defined(BT_USE_NEON)

	float32x4_t vQ1 = q1.get128();
	float32x4_t vQ2 = q2.get128();
	float32x4_t A0, A1, B1, A2, B2, A3, B3;
	float32x2_t vQ1zx, vQ2wx, vQ1yz, vQ2zx, vQ2yz, vQ2xz;

	{
		float32x2x2_t tmp;
		tmp = vtrn_f32(vget_high_f32(vQ1), vget_low_f32(vQ1));  
		vQ1zx = tmp.val[0];

		tmp = vtrn_f32(vget_high_f32(vQ2), vget_low_f32(vQ2));  
		vQ2zx = tmp.val[0];
	}
	vQ2wx = vext_f32(vget_high_f32(vQ2), vget_low_f32(vQ2), 1);

	vQ1yz = vext_f32(vget_low_f32(vQ1), vget_high_f32(vQ1), 1);

	vQ2yz = vext_f32(vget_low_f32(vQ2), vget_high_f32(vQ2), 1);
	vQ2xz = vext_f32(vQ2zx, vQ2zx, 1);

	A1 = vcombine_f32(vget_low_f32(vQ1), vQ1zx);                     
	B1 = vcombine_f32(vdup_lane_f32(vget_high_f32(vQ2), 1), vQ2wx);  

	A2 = vcombine_f32(vQ1yz, vget_low_f32(vQ1));
	B2 = vcombine_f32(vQ2zx, vdup_lane_f32(vget_low_f32(vQ2), 1));

	A3 = vcombine_f32(vQ1zx, vQ1yz);  
	B3 = vcombine_f32(vQ2yz, vQ2xz);  

	A1 = vmulq_f32(A1, B1);
	A2 = vmulq_f32(A2, B2);
	A3 = vmulq_f32(A3, B3);                           
	A0 = vmulq_lane_f32(vQ2, vget_high_f32(vQ1), 1);  

	A1 = vaddq_f32(A1, A2);  
	A0 = vsubq_f32(A0, A3);  

	
	A1 = (btSimdFloat4)veorq_s32((int32x4_t)A1, (int32x4_t)vPPPM);
	A0 = vaddq_f32(A0, A1);  

	return btQuaternion(A0);

#else
	return btQuaternion(
		q1.w() * q2.x() + q1.x() * q2.w() + q1.y() * q2.z() - q1.z() * q2.y(),
		q1.w() * q2.y() + q1.y() * q2.w() + q1.z() * q2.x() - q1.x() * q2.z(),
		q1.w() * q2.z() + q1.z() * q2.w() + q1.x() * q2.y() - q1.y() * q2.x(),
		q1.w() * q2.w() - q1.x() * q2.x() - q1.y() * q2.y() - q1.z() * q2.z());
#endif
}

SIMD_FORCE_INLINE btQuaternion
operator*(const btQuaternion& q, const btVector3& w)
{
#if defined(BT_USE_SSE_IN_API) && defined(BT_USE_SSE)
	__m128 vQ1 = q.get128();
	__m128 vQ2 = w.get128();
	__m128 A1, B1, A2, B2, A3, B3;

	A1 = bt_pshufd_ps(vQ1, BT_SHUFFLE(3, 3, 3, 0));
	B1 = bt_pshufd_ps(vQ2, BT_SHUFFLE(0, 1, 2, 0));

	A1 = A1 * B1;

	A2 = bt_pshufd_ps(vQ1, BT_SHUFFLE(1, 2, 0, 1));
	B2 = bt_pshufd_ps(vQ2, BT_SHUFFLE(2, 0, 1, 1));

	A2 = A2 * B2;

	A3 = bt_pshufd_ps(vQ1, BT_SHUFFLE(2, 0, 1, 2));
	B3 = bt_pshufd_ps(vQ2, BT_SHUFFLE(1, 2, 0, 2));

	A3 = A3 * B3;  

	A1 = A1 + A2;                
	A1 = _mm_xor_ps(A1, vPPPM);  
	A1 = A1 - A3;                

	return btQuaternion(A1);

#elif defined(BT_USE_NEON)

	float32x4_t vQ1 = q.get128();
	float32x4_t vQ2 = w.get128();
	float32x4_t A1, B1, A2, B2, A3, B3;
	float32x2_t vQ1wx, vQ2zx, vQ1yz, vQ2yz, vQ1zx, vQ2xz;

	vQ1wx = vext_f32(vget_high_f32(vQ1), vget_low_f32(vQ1), 1);
	{
		float32x2x2_t tmp;

		tmp = vtrn_f32(vget_high_f32(vQ2), vget_low_f32(vQ2));  
		vQ2zx = tmp.val[0];

		tmp = vtrn_f32(vget_high_f32(vQ1), vget_low_f32(vQ1));  
		vQ1zx = tmp.val[0];
	}

	vQ1yz = vext_f32(vget_low_f32(vQ1), vget_high_f32(vQ1), 1);

	vQ2yz = vext_f32(vget_low_f32(vQ2), vget_high_f32(vQ2), 1);
	vQ2xz = vext_f32(vQ2zx, vQ2zx, 1);

	A1 = vcombine_f32(vdup_lane_f32(vget_high_f32(vQ1), 1), vQ1wx);  
	B1 = vcombine_f32(vget_low_f32(vQ2), vQ2zx);                     

	A2 = vcombine_f32(vQ1yz, vget_low_f32(vQ1));
	B2 = vcombine_f32(vQ2zx, vdup_lane_f32(vget_low_f32(vQ2), 1));

	A3 = vcombine_f32(vQ1zx, vQ1yz);  
	B3 = vcombine_f32(vQ2yz, vQ2xz);  

	A1 = vmulq_f32(A1, B1);
	A2 = vmulq_f32(A2, B2);
	A3 = vmulq_f32(A3, B3);  

	A1 = vaddq_f32(A1, A2);  

	
	A1 = (btSimdFloat4)veorq_s32((int32x4_t)A1, (int32x4_t)vPPPM);

	A1 = vsubq_f32(A1, A3);  

	return btQuaternion(A1);

#else
	return btQuaternion(
		q.w() * w.x() + q.y() * w.z() - q.z() * w.y(),
		q.w() * w.y() + q.z() * w.x() - q.x() * w.z(),
		q.w() * w.z() + q.x() * w.y() - q.y() * w.x(),
		-q.x() * w.x() - q.y() * w.y() - q.z() * w.z());
#endif
}

SIMD_FORCE_INLINE btQuaternion
operator*(const btVector3& w, const btQuaternion& q)
{
#if defined(BT_USE_SSE_IN_API) && defined(BT_USE_SSE)
	__m128 vQ1 = w.get128();
	__m128 vQ2 = q.get128();
	__m128 A1, B1, A2, B2, A3, B3;

	A1 = bt_pshufd_ps(vQ1, BT_SHUFFLE(0, 1, 2, 0));  
	B1 = bt_pshufd_ps(vQ2, BT_SHUFFLE(3, 3, 3, 0));  

	A1 = A1 * B1;

	A2 = bt_pshufd_ps(vQ1, BT_SHUFFLE(1, 2, 0, 1));
	B2 = bt_pshufd_ps(vQ2, BT_SHUFFLE(2, 0, 1, 1));

	A2 = A2 * B2;

	A3 = bt_pshufd_ps(vQ1, BT_SHUFFLE(2, 0, 1, 2));
	B3 = bt_pshufd_ps(vQ2, BT_SHUFFLE(1, 2, 0, 2));

	A3 = A3 * B3;  

	A1 = A1 + A2;                
	A1 = _mm_xor_ps(A1, vPPPM);  
	A1 = A1 - A3;                

	return btQuaternion(A1);

#elif defined(BT_USE_NEON)

	float32x4_t vQ1 = w.get128();
	float32x4_t vQ2 = q.get128();
	float32x4_t A1, B1, A2, B2, A3, B3;
	float32x2_t vQ1zx, vQ2wx, vQ1yz, vQ2zx, vQ2yz, vQ2xz;

	{
		float32x2x2_t tmp;

		tmp = vtrn_f32(vget_high_f32(vQ1), vget_low_f32(vQ1));  
		vQ1zx = tmp.val[0];

		tmp = vtrn_f32(vget_high_f32(vQ2), vget_low_f32(vQ2));  
		vQ2zx = tmp.val[0];
	}
	vQ2wx = vext_f32(vget_high_f32(vQ2), vget_low_f32(vQ2), 1);

	vQ1yz = vext_f32(vget_low_f32(vQ1), vget_high_f32(vQ1), 1);

	vQ2yz = vext_f32(vget_low_f32(vQ2), vget_high_f32(vQ2), 1);
	vQ2xz = vext_f32(vQ2zx, vQ2zx, 1);

	A1 = vcombine_f32(vget_low_f32(vQ1), vQ1zx);                     
	B1 = vcombine_f32(vdup_lane_f32(vget_high_f32(vQ2), 1), vQ2wx);  

	A2 = vcombine_f32(vQ1yz, vget_low_f32(vQ1));
	B2 = vcombine_f32(vQ2zx, vdup_lane_f32(vget_low_f32(vQ2), 1));

	A3 = vcombine_f32(vQ1zx, vQ1yz);  
	B3 = vcombine_f32(vQ2yz, vQ2xz);  

	A1 = vmulq_f32(A1, B1);
	A2 = vmulq_f32(A2, B2);
	A3 = vmulq_f32(A3, B3);  

	A1 = vaddq_f32(A1, A2);  

	
	A1 = (btSimdFloat4)veorq_s32((int32x4_t)A1, (int32x4_t)vPPPM);

	A1 = vsubq_f32(A1, A3);  

	return btQuaternion(A1);

#else
	return btQuaternion(
		+w.x() * q.w() + w.y() * q.z() - w.z() * q.y(),
		+w.y() * q.w() + w.z() * q.x() - w.x() * q.z(),
		+w.z() * q.w() + w.x() * q.y() - w.y() * q.x(),
		-w.x() * q.x() - w.y() * q.y() - w.z() * q.z());
#endif
}


SIMD_FORCE_INLINE btScalar
dot(const btQuaternion& q1, const btQuaternion& q2)
{
	return q1.dot(q2);
}


SIMD_FORCE_INLINE btScalar
length(const btQuaternion& q)
{
	return q.length();
}


SIMD_FORCE_INLINE btScalar
btAngle(const btQuaternion& q1, const btQuaternion& q2)
{
	return q1.angle(q2);
}


SIMD_FORCE_INLINE btQuaternion
inverse(const btQuaternion& q)
{
	return q.inverse();
}


SIMD_FORCE_INLINE btQuaternion
slerp(const btQuaternion& q1, const btQuaternion& q2, const btScalar& t)
{
	return q1.slerp(q2, t);
}

SIMD_FORCE_INLINE btVector3
quatRotate(const btQuaternion& rotation, const btVector3& v)
{
	btQuaternion q = rotation * v;
	q *= rotation.inverse();
#if defined BT_USE_SIMD_VECTOR3 && defined(BT_USE_SSE_IN_API) && defined(BT_USE_SSE)
	return btVector3(_mm_and_ps(q.get128(), btvFFF0fMask));
#elif defined(BT_USE_NEON)
	return btVector3((float32x4_t)vandq_s32((int32x4_t)q.get128(), btvFFF0Mask));
#else
	return btVector3(q.getX(), q.getY(), q.getZ());
#endif
}

SIMD_FORCE_INLINE btQuaternion
shortestArcQuat(const btVector3& v0, const btVector3& v1)  
{
	btVector3 c = v0.cross(v1);
	btScalar d = v0.dot(v1);

	if (d < -1.0 + SIMD_EPSILON)
	{
		btVector3 n, unused;
		btPlaneSpace1(v0, n, unused);
		return btQuaternion(n.x(), n.y(), n.z(), 0.0f);  
	}

	btScalar s = btSqrt((1.0f + d) * 2.0f);
	btScalar rs = 1.0f / s;

	return btQuaternion(c.getX() * rs, c.getY() * rs, c.getZ() * rs, s * 0.5f);
}

SIMD_FORCE_INLINE btQuaternion
shortestArcQuatNormalize2(btVector3& v0, btVector3& v1)
{
	v0.normalize();
	v1.normalize();
	return shortestArcQuat(v0, v1);
}

struct btQuaternionFloatData
{
	float m_floats[4];
};

struct btQuaternionDoubleData
{
	double m_floats[4];
};

SIMD_FORCE_INLINE void btQuaternion::serializeFloat(struct btQuaternionFloatData& dataOut) const
{
	
	for (int i = 0; i < 4; i++)
		dataOut.m_floats[i] = float(m_floats[i]);
}

SIMD_FORCE_INLINE void btQuaternion::deSerializeFloat(const struct btQuaternionFloatData& dataIn)
{
	for (int i = 0; i < 4; i++)
		m_floats[i] = btScalar(dataIn.m_floats[i]);
}

SIMD_FORCE_INLINE void btQuaternion::serializeDouble(struct btQuaternionDoubleData& dataOut) const
{
	
	for (int i = 0; i < 4; i++)
		dataOut.m_floats[i] = double(m_floats[i]);
}

SIMD_FORCE_INLINE void btQuaternion::deSerializeDouble(const struct btQuaternionDoubleData& dataIn)
{
	for (int i = 0; i < 4; i++)
		m_floats[i] = btScalar(dataIn.m_floats[i]);
}

SIMD_FORCE_INLINE void btQuaternion::serialize(struct btQuaternionData& dataOut) const
{
	
	for (int i = 0; i < 4; i++)
		dataOut.m_floats[i] = m_floats[i];
}

SIMD_FORCE_INLINE void btQuaternion::deSerialize(const struct btQuaternionFloatData& dataIn)
{
	for (int i = 0; i < 4; i++)
		m_floats[i] = (btScalar)dataIn.m_floats[i];
}

SIMD_FORCE_INLINE void btQuaternion::deSerialize(const struct btQuaternionDoubleData& dataIn)
{
	for (int i = 0; i < 4; i++)
		m_floats[i] = (btScalar)dataIn.m_floats[i];
}

#endif  
