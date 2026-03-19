



#ifndef __I_IRRKLANG_SOUND_DEVICE_LIST_H_INCLUDED__
#define __I_IRRKLANG_SOUND_DEVICE_LIST_H_INCLUDED__

#include "ik_IRefCounted.h"

namespace irrklang
{



class ISoundDeviceList : public IRefCounted
{
public:

	
	virtual ik_s32 getDeviceCount() = 0;

	
	
	virtual const char* getDeviceID(ik_s32 index) = 0;

	
	
	virtual const char* getDeviceDescription(ik_s32 index) = 0;
};


} 

#endif

