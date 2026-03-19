



#ifndef __I_IRRKLANG_AUDIO_RECORDER_H_INCLUDED__
#define __I_IRRKLANG_AUDIO_RECORDER_H_INCLUDED__

#include "ik_IRefCounted.h"
#include "ik_ISoundSource.h"


namespace irrklang
{
	class ICapturedAudioDataReceiver;

	
	
	class IAudioRecorder : public virtual IRefCounted
	{
	public:

		
		
		virtual bool startRecordingBufferedAudio(ik_s32 sampleRate=22000, 
		                                         ESampleFormat sampleFormat=ESF_S16,
												 ik_s32 channelCount=1) = 0;

		
		
		virtual bool startRecordingCustomHandledAudio(ICapturedAudioDataReceiver* receiver,
			                                          ik_s32 sampleRate=22000,
													  ESampleFormat sampleFormat=ESF_S16,
													  ik_s32 channelCount=1) = 0;

		
		virtual void stopRecordingAudio() = 0;

		
		
		virtual ISoundSource* addSoundSourceFromRecordedAudio(const char* soundName) = 0;

		
		
		virtual void clearRecordedAudioDataBuffer() = 0;

		
		virtual bool isRecording() = 0;

		
		
		virtual SAudioStreamFormat getAudioFormat() = 0;

		
		
		virtual void* getRecordedAudioData() = 0;

		
		
		virtual const char* getDriverName() = 0;
	};


	
	
	class ICapturedAudioDataReceiver : public IRefCounted
	{
	public:

		
		
		virtual void OnReceiveAudioDataStreamChunk(unsigned char* audioData, unsigned long lengthInBytes) = 0;
	};


} 


#endif
