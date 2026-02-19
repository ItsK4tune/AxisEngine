



#ifndef __I_IRRKLANG_VIRTUAL_UNKNOWN_H_INCLUDED__
#define __I_IRRKLANG_VIRTUAL_UNKNOWN_H_INCLUDED__

#include "ik_irrKlangTypes.h"


namespace irrklang
{

	
	
	class IVirtualRefCounted
	{
	public:

		
		virtual ~IVirtualRefCounted()
		{
		}

		
		
		virtual void grab() = 0;

		
		
		virtual bool drop() = 0;
	};



} 



#endif

