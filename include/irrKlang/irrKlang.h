

#ifndef __IRR_KLANG_H_INCLUDED__
#define __IRR_KLANG_H_INCLUDED__

#include "ik_irrKlangTypes.h"
#include "ik_vec3d.h"

#include "ik_IRefCounted.h"
#include "ik_IVirtualRefCounted.h"

#include "ik_ESoundOutputDrivers.h"
#include "ik_ESoundEngineOptions.h"
#include "ik_EStreamModes.h"
#include "ik_SAudioStreamFormat.h"
#include "ik_ISoundEngine.h"
#include "ik_ISoundSource.h"
#include "ik_ISound.h"
#include "ik_IAudioStream.h"
#include "ik_IAudioStreamLoader.h"
#include "ik_ISoundEffectControl.h"
#include "ik_ISoundStopEventReceiver.h"
#include "ik_IFileFactory.h"
#include "ik_IFileReader.h"
#include "ik_ISoundDeviceList.h"
#include "ik_IAudioRecorder.h"
#include "ik_ISoundMixedOutputReceiver.h"


#define IRR_KLANG_VERSION "1.6.0"



#if defined(IRRKLANG_STATIC)
    #define IRRKLANG_API
#else
    #if (defined(WIN32) || defined(WIN64) || defined(_MSC_VER))
        #ifdef IRRKLANG_EXPORTS
        #define IRRKLANG_API __declspec(dllexport)
        #else
        #define IRRKLANG_API __declspec(dllimport)
        #endif 
    #else
        #define IRRKLANG_API __attribute__((visibility("default")))
    #endif 
#endif 

#if defined(_STDCALL_SUPPORTED)
#define IRRKLANGCALLCONV __stdcall  
#else
#define IRRKLANGCALLCONV
#endif 


namespace irrklang
{
	
	
	IRRKLANG_API ISoundEngine* IRRKLANGCALLCONV createIrrKlangDevice(
		E_SOUND_OUTPUT_DRIVER driver = ESOD_AUTO_DETECT,
		int options = ESEO_DEFAULT_OPTIONS,
		const char* deviceID = 0,
		const char* sdk_version_do_not_use = IRR_KLANG_VERSION);


	
	
	IRRKLANG_API ISoundDeviceList* IRRKLANGCALLCONV createSoundDeviceList(
		E_SOUND_OUTPUT_DRIVER driver = ESOD_AUTO_DETECT,
		const char* sdk_version_do_not_use = IRR_KLANG_VERSION);


	
	
	IRRKLANG_API IAudioRecorder* IRRKLANGCALLCONV createIrrKlangAudioRecorder(
		ISoundEngine* irrKlangDeviceForPlayback,
		E_SOUND_OUTPUT_DRIVER driver = ESOD_AUTO_DETECT,
		const char* deviceID = 0,
		const char* sdk_version_do_not_use = IRR_KLANG_VERSION);

	
	
	IRRKLANG_API ISoundDeviceList* IRRKLANGCALLCONV createAudioRecorderDeviceList(
		E_SOUND_OUTPUT_DRIVER driver = ESOD_AUTO_DETECT,
		const char* sdk_version_do_not_use = IRR_KLANG_VERSION);


	
	
	IRRKLANG_API bool IRRKLANGCALLCONV makeUTF8fromUTF16string(
		const wchar_t* pInputString, char* pOutputBuffer, int outputBufferSize);


} 




#endif

