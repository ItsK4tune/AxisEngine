



#ifndef __I_IRRKLANG_SOUND_H_INCLUDED__
#define __I_IRRKLANG_SOUND_H_INCLUDED__

#include "ik_IVirtualRefCounted.h"
#include "ik_ISoundEffectControl.h"
#include "ik_vec3d.h"


namespace irrklang
{
	class ISoundSource;
	class ISoundStopEventReceiver;

	
	
	class ISound : public IVirtualRefCounted
	{
	public:

		
		
		virtual ISoundSource* getSoundSource() = 0;

		
		virtual void setIsPaused( bool paused = true) = 0;

		
		virtual bool getIsPaused() = 0;

		
		
		virtual void stop() = 0;

		
		
		virtual ik_f32 getVolume() = 0;

		
		
		virtual void setVolume(ik_f32 volume) = 0;

		
		virtual void setPan(ik_f32 pan) = 0;

		
		virtual ik_f32 getPan() = 0;

		
		virtual bool isLooped() = 0;

		
		
		virtual void setIsLooped(bool looped) = 0;

		
		
		virtual bool isFinished() = 0;

		
		
		virtual void setMinDistance(ik_f32 min) = 0;

		
		
		virtual ik_f32 getMinDistance() = 0;

		
		
		virtual void setMaxDistance(ik_f32 max) = 0;

		
		
		virtual ik_f32 getMaxDistance() = 0;

		
		virtual void setPosition(vec3df position) = 0;

		
		virtual vec3df getPosition() = 0;

		
		
		virtual void setVelocity(vec3df vel) = 0;

		
		
		virtual vec3df getVelocity() = 0;

		
		
		virtual ik_u32 getPlayPosition() = 0;

		
        
		virtual bool setPlayPosition(ik_u32 pos) = 0;

		
		
		virtual bool setPlaybackSpeed(ik_f32 speed = 1.0f) = 0;

		
		
		virtual ik_f32 getPlaybackSpeed() = 0;

		
		
		virtual ik_u32 getPlayLength() = 0;

		
		
		virtual ISoundEffectControl* getSoundEffectControl() = 0;

		
		
		virtual void setSoundStopEventReceiver(ISoundStopEventReceiver* receiver, void* userData=0) = 0;
	};

} 


#endif
