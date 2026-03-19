



#ifndef __I_IRRKLANG_AUDIO_STREAM_H_INCLUDED__
#define __I_IRRKLANG_AUDIO_STREAM_H_INCLUDED__

#include "ik_IRefCounted.h"
#include "ik_SAudioStreamFormat.h"

namespace irrklang
{



class IAudioStream : public IRefCounted
{
public:

	
	virtual ~IAudioStream() {};

	
	virtual SAudioStreamFormat getFormat() = 0;

	
	
	virtual bool setPosition(ik_s32 pos) = 0;

	
	
	virtual bool getIsSeekingSupported() { return true; }

    
	
	virtual ik_s32 readFrames(void* target, ik_s32 frameCountToRead) = 0;
};


} 

#endif

