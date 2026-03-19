



#ifndef __I_IRRKLANG_IREFERENCE_COUNTED_H_INCLUDED__
#define __I_IRRKLANG_IREFERENCE_COUNTED_H_INCLUDED__

#include "ik_irrKlangTypes.h"

namespace irrklang
{
	
	
	class IRefCounted
	{
	public:

		
		IRefCounted()
			: ReferenceCounter(1)
		{
		}

		
		virtual ~IRefCounted()
		{
		}

		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		void grab() { ++ReferenceCounter; }

		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		bool drop()
		{
			--ReferenceCounter;

			if (!ReferenceCounter)
			{
				delete this;
				return true;
			}

			return false;
		}

	private:

		ik_s32	ReferenceCounter;
	};

} 

#endif

