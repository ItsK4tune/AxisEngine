



#ifndef __I_IRRKLANG_IRR_SOUND_SOURCE_H_INCLUDED__
#define __I_IRRKLANG_IRR_SOUND_SOURCE_H_INCLUDED__

#include "ik_IVirtualRefCounted.h"
#include "ik_vec3d.h"
#include "ik_EStreamModes.h"
#include "ik_SAudioStreamFormat.h"


namespace irrklang
{

	
	
	class ISoundSource : public IVirtualRefCounted
	{
	public:

		
		virtual const ik_c8* getName() = 0;

		
		
		virtual void setStreamMode(E_STREAM_MODE mode) = 0;

		
		
		virtual E_STREAM_MODE getStreamMode() = 0;

		
		
		virtual ik_u32 getPlayLength() = 0;

		
		
		virtual SAudioStreamFormat getAudioFormat() = 0;

		
		
		virtual bool getIsSeekingSupported() = 0;

		
		
		virtual void setDefaultVolume(ik_f32 volume=1.0f) = 0;

		
		
		virtual ik_f32 getDefaultVolume() = 0;

		
		
		virtual void setDefaultMinDistance(ik_f32 minDistance) = 0;

		
		
		virtual ik_f32 getDefaultMinDistance() = 0;

		
		
		virtual void setDefaultMaxDistance(ik_f32 maxDistance) = 0;

		
		
		virtual ik_f32 getDefaultMaxDistance() = 0;

		
		
		virtual void forceReloadAtNextUse() = 0;

		
		
		virtual void setForcedStreamingThreshold(ik_s32 thresholdBytes) = 0;

		
		
		virtual ik_s32 getForcedStreamingThreshold() = 0;

		
		
		virtual void* getSampleData() = 0;
	};

} 


#endif
