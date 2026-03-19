



#ifndef __I_IRRKLANG_SOUND_STOP_EVENT_RECEIVER_H_INCLUDED__
#define __I_IRRKLANG_SOUND_STOP_EVENT_RECEIVER_H_INCLUDED__

#include "ik_IRefCounted.h"
#include "ik_SAudioStreamFormat.h"


namespace irrklang
{



enum E_STOP_EVENT_CAUSE
{
	
	ESEC_SOUND_FINISHED_PLAYING = 0,

	
	ESEC_SOUND_STOPPED_BY_USER,

	
	
	ESEC_SOUND_STOPPED_BY_SOURCE_REMOVAL,

	
	
	ESEC_FORCE_32_BIT = 0x7fffffff
};




class ISoundStopEventReceiver
{
public:
    
	
	virtual ~ISoundStopEventReceiver() {};

	
	
	virtual void OnSoundStopped(ISound* sound, E_STOP_EVENT_CAUSE reason, void* userData) = 0;

};


} 


#endif

