



#ifndef __I_IRRKLANG_FILE_FACTORY_H_INCLUDED__
#define __I_IRRKLANG_FILE_FACTORY_H_INCLUDED__

#include "ik_IRefCounted.h"

namespace irrklang
{
	class IFileReader;

	
	
	class IFileFactory : public virtual IRefCounted
	{
	public:

		virtual ~IFileFactory() {};

		
		
		virtual IFileReader* createFileReader(const ik_c8* filename) = 0;		
	};

} 

#endif

