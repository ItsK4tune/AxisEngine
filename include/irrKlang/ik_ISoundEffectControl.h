



#ifndef __I_IRRKLANG_SOUND_EFFECT_CONTROL_H_INCLUDED__
#define __I_IRRKLANG_SOUND_EFFECT_CONTROL_H_INCLUDED__

#include "ik_IVirtualRefCounted.h"
#include "ik_vec3d.h"


namespace irrklang
{
	
	
	class ISoundEffectControl
	{
	public:

		
		virtual void disableAllEffects() = 0;

		
		
		virtual bool enableChorusSoundEffect(ik_f32 fWetDryMix = 50,
											ik_f32 fDepth = 10,
											ik_f32 fFeedback = 25,
											ik_f32 fFrequency = 1.1,
											bool sinusWaveForm = true,
											ik_f32 fDelay = 16,
											ik_s32 lPhase = 90) = 0;

		
		virtual void disableChorusSoundEffect() = 0;

		
		virtual bool isChorusSoundEffectEnabled() = 0;

		
		
		virtual bool enableCompressorSoundEffect( ik_f32 fGain = 0,
												ik_f32 fAttack = 10,
												ik_f32 fRelease = 200,
												ik_f32 fThreshold = -20,
												ik_f32 fRatio = 3,
												ik_f32 fPredelay = 4) = 0;

		
		virtual void disableCompressorSoundEffect() = 0;

		
		virtual bool isCompressorSoundEffectEnabled() = 0;

		
		
		virtual bool enableDistortionSoundEffect(ik_f32 fGain = -18,
												ik_f32 fEdge = 15,
												ik_f32 fPostEQCenterFrequency = 2400,
												ik_f32 fPostEQBandwidth = 2400,
												ik_f32 fPreLowpassCutoff = 8000) = 0;

		
		virtual void disableDistortionSoundEffect() = 0;

		
		virtual bool isDistortionSoundEffectEnabled() = 0;

		
		
		virtual bool enableEchoSoundEffect(ik_f32 fWetDryMix = 50,
											ik_f32 fFeedback = 50,
											ik_f32 fLeftDelay = 500,
											ik_f32 fRightDelay = 500,
											ik_s32 lPanDelay = 0) = 0;

		
		virtual void disableEchoSoundEffect() = 0;

		
		virtual bool isEchoSoundEffectEnabled() = 0;

		
		
		virtual bool enableFlangerSoundEffect(ik_f32 fWetDryMix = 50,
											ik_f32 fDepth = 100,
											ik_f32 fFeedback = -50,
											ik_f32 fFrequency = 0.25f,
											bool triangleWaveForm = true,
											ik_f32 fDelay = 2,
											ik_s32 lPhase = 0) = 0;

		
		virtual void disableFlangerSoundEffect() = 0;

		
		virtual bool isFlangerSoundEffectEnabled() = 0;

		
		
		virtual bool enableGargleSoundEffect(ik_s32 rateHz = 20, bool sinusWaveForm = true) = 0;

		
		virtual void disableGargleSoundEffect() = 0;

		
		virtual bool isGargleSoundEffectEnabled() = 0;

		
		
		virtual bool enableI3DL2ReverbSoundEffect(ik_s32 lRoom = -1000,
												ik_s32 lRoomHF = -100,
												ik_f32 flRoomRolloffFactor = 0,
												ik_f32 flDecayTime = 1.49f,
												ik_f32 flDecayHFRatio = 0.83f,
												ik_s32 lReflections = -2602,
												ik_f32 flReflectionsDelay = 0.007f,
												ik_s32 lReverb = 200,
												ik_f32 flReverbDelay = 0.011f,
												ik_f32 flDiffusion = 100.0f,
												ik_f32 flDensity = 100.0f,
												ik_f32 flHFReference = 5000.0f ) = 0;

		
		virtual void disableI3DL2ReverbSoundEffect() = 0;

		
		virtual bool isI3DL2ReverbSoundEffectEnabled() = 0;

		
		
		virtual bool enableParamEqSoundEffect(ik_f32 fCenter = 8000,
											ik_f32 fBandwidth = 12,
											ik_f32 fGain = 0) = 0;

		
		virtual void disableParamEqSoundEffect() = 0;

		
		virtual bool isParamEqSoundEffectEnabled() = 0;

		
		
		virtual bool enableWavesReverbSoundEffect(ik_f32 fInGain = 0,
											ik_f32 fReverbMix = 0,
											ik_f32 fReverbTime = 1000,
											ik_f32 fHighFreqRTRatio = 0.001f) = 0;

		
		virtual void disableWavesReverbSoundEffect() = 0;

		
		virtual bool isWavesReverbSoundEffectEnabled() = 0;
	};

} 


#endif
