



#ifndef __I_IRRKLANG_SOUND_MIXED_OUTPUT_RECEIVER_H_INCLUDED__
#define __I_IRRKLANG_SOUND_MIXED_OUTPUT_RECEIVER_H_INCLUDED__

#include "ik_IRefCounted.h"
#include "ik_SAudioStreamFormat.h"


namespace irrklang
{




class ISoundMixedOutputReceiver
{
public:
    
	
	virtual ~ISoundMixedOutputReceiver() {};

	
	
	virtual void OnAudioDataReady(const void* data, int byteCount, int playbackrate) = 0;

};


} 


#endif

