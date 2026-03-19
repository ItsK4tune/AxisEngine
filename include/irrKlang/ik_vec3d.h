



#ifndef __IRR_IRRKLANG_VEC_3D_H_INCLUDED__
#define __IRR_IRRKLANG_VEC_3D_H_INCLUDED__

#include <math.h>
#include "ik_irrKlangTypes.h"


namespace irrklang
{

	
	template <class T>
	class vec3d
	{
	public:

		vec3d(): X(0), Y(0), Z(0) {};
		vec3d(T nx, T ny, T nz) : X(nx), Y(ny), Z(nz) {};
		vec3d(const vec3d<T>& other)	:X(other.X), Y(other.Y), Z(other.Z) {};

		
		#ifdef __IRR_POINT_3D_H_INCLUDED__
		template<class B>
		vec3d(const B& other)	:X(other.X), Y(other.Y), Z(other.Z) {};
		#endif 

		

		vec3d<T> operator-() const { return vec3d<T>(-X, -Y, -Z);   }

		vec3d<T>& operator=(const vec3d<T>& other)	{ X = other.X; Y = other.Y; Z = other.Z; return *this; }

		vec3d<T> operator+(const vec3d<T>& other) const { return vec3d<T>(X + other.X, Y + other.Y, Z + other.Z);	}
		vec3d<T>& operator+=(const vec3d<T>& other)	{ X+=other.X; Y+=other.Y; Z+=other.Z; return *this; }

		vec3d<T> operator-(const vec3d<T>& other) const { return vec3d<T>(X - other.X, Y - other.Y, Z - other.Z);	}
		vec3d<T>& operator-=(const vec3d<T>& other)	{ X-=other.X; Y-=other.Y; Z-=other.Z; return *this; }

		vec3d<T> operator*(const vec3d<T>& other) const { return vec3d<T>(X * other.X, Y * other.Y, Z * other.Z);	}
		vec3d<T>& operator*=(const vec3d<T>& other)	{ X*=other.X; Y*=other.Y; Z*=other.Z; return *this; }
		vec3d<T> operator*(const T v) const { return vec3d<T>(X * v, Y * v, Z * v);	}
		vec3d<T>& operator*=(const T v) { X*=v; Y*=v; Z*=v; return *this; }

		vec3d<T> operator/(const vec3d<T>& other) const { return vec3d<T>(X / other.X, Y / other.Y, Z / other.Z);	}
		vec3d<T>& operator/=(const vec3d<T>& other)	{ X/=other.X; Y/=other.Y; Z/=other.Z; return *this; }
		vec3d<T> operator/(const T v) const { T i=(T)1.0/v; return vec3d<T>(X * i, Y * i, Z * i);	}
		vec3d<T>& operator/=(const T v) { T i=(T)1.0/v; X*=i; Y*=i; Z*=i; return *this; }

		bool operator<=(const vec3d<T>&other) const { return X<=other.X && Y<=other.Y && Z<=other.Z;};
		bool operator>=(const vec3d<T>&other) const { return X>=other.X && Y>=other.Y && Z>=other.Z;};

		bool operator==(const vec3d<T>& other) const { return other.X==X && other.Y==Y && other.Z==Z; }
		bool operator!=(const vec3d<T>& other) const { return other.X!=X || other.Y!=Y || other.Z!=Z; }

		

		
		bool equals(const vec3d<T>& other)
		{
			return equalsfloat(X, other.X) &&
				   equalsfloat(Y, other.Y) &&
				   equalsfloat(Z, other.Z);
		}

		void set(const T nx, const T ny, const T nz) {X=nx; Y=ny; Z=nz; }
		void set(const vec3d<T>& p) { X=p.X; Y=p.Y; Z=p.Z;}

		
		ik_f64 getLength() const { return sqrt(X*X + Y*Y + Z*Z); }

		
		
		ik_f64 getLengthSQ() const { return X*X + Y*Y + Z*Z; }

		
		T dotProduct(const vec3d<T>& other) const
		{
			return X*other.X + Y*other.Y + Z*other.Z;
		}

		
		
		ik_f64 getDistanceFrom(const vec3d<T>& other) const
		{
			ik_f64 vx = X - other.X; ik_f64 vy = Y - other.Y; ik_f64 vz = Z - other.Z;
			return sqrt(vx*vx + vy*vy + vz*vz);
		}

		
		
		ik_f32 getDistanceFromSQ(const vec3d<T>& other) const
		{
			ik_f32 vx = X - other.X; ik_f32 vy = Y - other.Y; ik_f32 vz = Z - other.Z;
			return (vx*vx + vy*vy + vz*vz);
		}

		
		vec3d<T> crossProduct(const vec3d<T>& p) const
		{
			return vec3d<T>(Y * p.Z - Z * p.Y, Z * p.X - X * p.Z, X * p.Y - Y * p.X);
		}

		
		
		bool isBetweenPoints(const vec3d<T>& begin, const vec3d<T>& end) const
		{
			ik_f32 f = (ik_f32)(end - begin).getLengthSQ();
			return (ik_f32)getDistanceFromSQ(begin) < f &&
				(ik_f32)getDistanceFromSQ(end) < f;
		}

		
		vec3d<T>& normalize()
		{
			T l = (T)getLength();
			if (l == 0)
				return *this;

			l = (T)1.0 / l;
			X *= l;
			Y *= l;
			Z *= l;
			return *this;
		}

		
		void setLength(T newlength)
		{
			normalize();
			*this *= newlength;
		}

		
		void invert()
		{
			X *= -1.0f;
			Y *= -1.0f;
			Z *= -1.0f;
		}

		
		
		
		
		void rotateXZBy(ik_f64 degrees, const vec3d<T>& center)
		{
			degrees *= IK_DEGTORAD64;
			T cs = (T)cos(degrees);
			T sn = (T)sin(degrees);
			X -= center.X;
			Z -= center.Z;
			set(X*cs - Z*sn, Y, X*sn + Z*cs);
			X += center.X;
			Z += center.Z;
		}

		
		
		
		
		void rotateXYBy(ik_f64 degrees, const vec3d<T>& center)
		{
			degrees *= IK_DEGTORAD64;
			T cs = (T)cos(degrees);
			T sn = (T)sin(degrees);
			X -= center.X;
			Y -= center.Y;
			set(X*cs - Y*sn, X*sn + Y*cs, Z);
			X += center.X;
			Y += center.Y;
		}

		
		
		
		
		void rotateYZBy(ik_f64 degrees, const vec3d<T>& center)
		{
			degrees *= IK_DEGTORAD64;
			T cs = (T)cos(degrees);
			T sn = (T)sin(degrees);
			Z -= center.Z;
			Y -= center.Y;
			set(X, Y*cs - Z*sn, Y*sn + Z*cs);
			Z += center.Z;
			Y += center.Y;
		}

		
		
		vec3d<T> getInterpolated(const vec3d<T>& other, ik_f32 d) const
		{
			ik_f32 inv = 1.0f - d;
			return vec3d<T>(other.X*inv + X*d,
								other.Y*inv + Y*d,
								other.Z*inv + Z*d);
		}

		
		
		vec3d<T> getHorizontalAngle()
		{
			vec3d<T> angle;

			angle.Y = (T)atan2(X, Z);
			angle.Y *= (ik_f32)IK_RADTODEG;

			if (angle.Y < 0.0f) angle.Y += 360.0f;
			if (angle.Y >= 360.0f) angle.Y -= 360.0f;

			ik_f32 z1 = (T)sqrt(X*X + Z*Z);

			angle.X = (T)atan2(z1, Y);
			angle.X *= (ik_f32)IK_RADTODEG;
			angle.X -= 90.0f;

			if (angle.X < 0.0f) angle.X += 360.0f;
			if (angle.X >= 360) angle.X -= 360.0f;

			return angle;
		}

		
		
		void getAs4Values(T* array)
		{
			array[0] = X;
			array[1] = Y;
			array[2] = Z;
			array[3] = 0;
		}


		

		T X, Y, Z;
	};


	
	typedef vec3d<ik_f32> vec3df;

	
	typedef vec3d<ik_s32> vec3di;

	template<class S, class T> vec3d<T> operator*(const S scalar, const vec3d<T>& vector) { return vector*scalar; }

} 


#endif

