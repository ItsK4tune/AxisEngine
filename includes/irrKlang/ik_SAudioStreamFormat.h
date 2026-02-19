



#ifndef __S_IRRKLANG_AUDIO_STREAM_FORMAT_H_INCLUDED__
#define __S_IRRKLANG_AUDIO_STREAM_FORMAT_H_INCLUDED__

#include "ik_IRefCounted.h"


namespace irrklang
{

	
	enum ESampleFormat
	{
		
		ESF_U8, 

		
		ESF_S16 
	};


	
	struct SAudioStreamFormat
	{
		
		ik_s32 ChannelCount; 

		
		
		ik_s32 FrameCount;		

		
		ik_s32 SampleRate;
		
		
		ESampleFormat SampleFormat;

		
		inline ik_s32 getSampleSize() const
		{
			return (SampleFormat == ESF_U8) ? 1 : 2;
		}

		
		inline ik_s32 getFrameSize() const
		{
			return ChannelCount * getSampleSize();
		}

		
		
		inline ik_s32 getSampleDataSize() const
		{
			return getFrameSize() * FrameCount;
		}

		
		inline ik_s32 getBytesPerSecond() const
		{
			return getFrameSize() * SampleRate;
		}
	};


} 

#endif

