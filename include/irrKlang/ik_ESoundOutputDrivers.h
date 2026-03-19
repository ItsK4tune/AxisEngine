



#ifndef __E_IRRKLANG_SOUND_OUTPUT_DRIVERS_H_INCLUDED__
#define __E_IRRKLANG_SOUND_OUTPUT_DRIVERS_H_INCLUDED__

namespace irrklang
{
	
	
	enum E_SOUND_OUTPUT_DRIVER
	{
		
		ESOD_AUTO_DETECT = 0,

		
		
		ESOD_DIRECT_SOUND_8,

		
		
		ESOD_DIRECT_SOUND,

		
		
		ESOD_WIN_MM,

		
		
		ESOD_ALSA,
		
		
		
		ESOD_CORE_AUDIO,

		
		ESOD_NULL,

		
		ESOD_COUNT,

		
		
		ESOD_FORCE_32_BIT = 0x7fffffff
	};

} 

#endif

