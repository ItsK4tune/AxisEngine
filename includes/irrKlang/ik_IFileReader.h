



#ifndef __I_IRRKLANG_READ_FILE_H_INCLUDED__
#define __I_IRRKLANG_READ_FILE_H_INCLUDED__

#include "ik_IRefCounted.h"

namespace irrklang
{

	
	class IFileReader : public virtual IRefCounted
	{
	public:

		virtual ~IFileReader() {};

		
		
		
		
		virtual ik_s32 read(void* buffer, ik_u32 sizeToRead) = 0;

		
		
		
		
		
		
		virtual bool seek(ik_s32 finalPos, bool relativeMovement = false) = 0;

		
		
		virtual ik_s32 getSize() = 0;

		
		
		virtual ik_s32 getPos() = 0;

		
		
		virtual const ik_c8* getFileName() = 0;
	};

} 

#endif

