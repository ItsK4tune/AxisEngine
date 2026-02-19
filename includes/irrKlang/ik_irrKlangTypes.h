



#ifndef __IRRKLANG_TYPES_H_INCLUDED__
#define __IRRKLANG_TYPES_H_INCLUDED__


namespace irrklang
{

	
	
	typedef unsigned char ik_u8;

	
	
	typedef signed char	ik_s8;

	
	
	typedef char ik_c8;



	
	
	typedef unsigned short ik_u16;

	
	
	typedef signed short ik_s16;



	
	
	typedef unsigned int ik_u32;

	
	
	typedef signed int ik_s32;



	
	
	typedef float ik_f32;

	
	
	typedef double ik_f64;



    

	const ik_f32 IK_ROUNDING_ERROR_32	= 0.000001f;
	const ik_f64 IK_PI64			    = 3.1415926535897932384626433832795028841971693993751;
	const ik_f32 IK_PI32			    = 3.14159265359f;
	const ik_f32 IK_RADTODEG            = 180.0f / IK_PI32;
	const ik_f32 IK_DEGTORAD            = IK_PI32 / 180.0f;
	const ik_f64 IK_RADTODEG64          = 180.0 / IK_PI64;
	const ik_f64 IK_DEGTORAD64          = IK_PI64 / 180.0;

	
	
	inline bool equalsfloat(const ik_f32 a, const ik_f32 b, const ik_f32 tolerance = IK_ROUNDING_ERROR_32)
	{
		return (a + tolerance > b) && (a - tolerance < b);
	}

} 


#include <wchar.h>


#ifdef _MSC_VER  
	#ifndef _WCHAR_T_DEFINED
		
		
		typedef unsigned short wchar_t;
		#define _WCHAR_T_DEFINED
	#endif 
#endif 


#endif 

