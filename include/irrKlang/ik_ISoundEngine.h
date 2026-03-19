



#ifndef __I_IRRKLANG_SOUND_ENGINE_H_INCLUDED__
#define __I_IRRKLANG_SOUND_ENGINE_H_INCLUDED__

#include "ik_IRefCounted.h"
#include "ik_vec3d.h"
#include "ik_ISoundSource.h"
#include "ik_ISound.h"
#include "ik_EStreamModes.h"
#include "ik_IFileFactory.h"
#include "ik_ISoundMixedOutputReceiver.h"


namespace irrklang
{
	class IAudioStreamLoader;
	struct SInternalAudioInterface;

	
	
	class ISoundEngine : public virtual irrklang::IRefCounted
	{
	public:

		
		
		virtual const char* getDriverName() = 0;

		
		
		virtual ISound* play2D(const char* soundFileName, 
							   bool playLooped = false,
							   bool startPaused = false, 
							   bool track = false,
							   E_STREAM_MODE streamMode = ESM_AUTO_DETECT,
							   bool enableSoundEffects = false) = 0;

		
		
		virtual ISound* play2D(ISoundSource* source, 
							   bool playLooped = false,
							   bool startPaused = false, 
							   bool track = false,
							   bool enableSoundEffects = false) = 0;

		
		
		virtual ISound* play3D(const char* soundFileName, vec3df pos,
							   bool playLooped = false, 
							   bool startPaused = false,
							   bool track = false, 
							   E_STREAM_MODE streamMode = ESM_AUTO_DETECT,
							   bool enableSoundEffects = false) = 0;

		
		
		virtual ISound* play3D(ISoundSource* source, vec3df pos,
							   bool playLooped = false, 
							   bool startPaused = false, 
							   bool track = false,
							   bool enableSoundEffects = false) = 0;

		
		virtual void stopAllSounds() = 0;

        
		virtual void setAllSoundsPaused( bool bPaused = true ) = 0;

		
		
		virtual ISoundSource* getSoundSource(const ik_c8* soundName, bool addIfNotFound=true) = 0;

		
			
		virtual ISoundSource* getSoundSource(ik_s32 index) = 0;

		
		virtual ik_s32 getSoundSourceCount() = 0;

		
			
		virtual ISoundSource* addSoundSourceFromFile(const ik_c8* fileName, E_STREAM_MODE mode=ESM_AUTO_DETECT,
			                                         bool preload=false) = 0;

		
		
		virtual ISoundSource* addSoundSourceFromMemory(void* memory, ik_s32 sizeInBytes, const ik_c8* soundName,
											               bool copyMemory=true) = 0;


		
		
		virtual ISoundSource* addSoundSourceFromPCMData(void* memory, ik_s32 sizeInBytes, 
			                                            const ik_c8* soundName, SAudioStreamFormat format,
														bool copyMemory=true) = 0;

		
		
		virtual ISoundSource* addSoundSourceAlias(ISoundSource* baseSource, const ik_c8* soundName) = 0;

		
		
		virtual void removeSoundSource(ISoundSource* source) = 0;

		
		
		virtual void removeSoundSource(const ik_c8* name) = 0;

		
		
		virtual void removeAllSoundSources() = 0;

		
		
		virtual void setSoundVolume(ik_f32 volume) = 0;

		
		
		virtual ik_f32 getSoundVolume() = 0;

		
		
		virtual void setListenerPosition(const vec3df& pos,
			const vec3df& lookdir,
			const vec3df& velPerSecond = vec3df(0,0,0),
			const vec3df& upVector = vec3df(0,1,0)) = 0;

		
		
		virtual void update() = 0;

		
		virtual bool isCurrentlyPlaying(const char* soundName) = 0;

		
		virtual bool isCurrentlyPlaying(ISoundSource* source) = 0;

		
		virtual void stopAllSoundsOfSoundSource(ISoundSource* source) = 0;

		
		
		virtual void registerAudioStreamLoader(IAudioStreamLoader* loader) = 0;

		
		
		virtual bool isMultiThreaded() const = 0;

		
		
		virtual void addFileFactory(IFileFactory* fileFactory) = 0;

		
		
		virtual void setDefault3DSoundMinDistance(ik_f32 minDistance) = 0;

		
		
		virtual ik_f32 getDefault3DSoundMinDistance() = 0;

		
		
		virtual void setDefault3DSoundMaxDistance(ik_f32 maxDistance) = 0;

		
		
		virtual ik_f32 getDefault3DSoundMaxDistance() = 0;

		
		
		virtual void setRolloffFactor(ik_f32 rolloff) = 0;

		
		
		virtual void setDopplerEffectParameters(ik_f32 dopplerFactor=1.0f, ik_f32 distanceFactor=1.0f) = 0;

		
		
		virtual bool loadPlugins(const ik_c8* path) = 0;

		
		
		virtual const SInternalAudioInterface& getInternalAudioInterface() = 0;		

		
		
		virtual bool setMixedDataOutputReceiver(ISoundMixedOutputReceiver* receiver) = 0;
	};


	
	
	struct SInternalAudioInterface
	{
		
		void* pIDirectSound;

		
		void* pIDirectSound8;

		
		void* pWinMM_HWaveOut;

		
		void* pALSA_SND_PCM;

		
		ik_u32 pCoreAudioDeviceID;
	};



} 


#endif
