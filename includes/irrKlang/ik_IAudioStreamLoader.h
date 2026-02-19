



#ifndef __I_IRRKLANG_AUDIO_STREAM_LOADER_H_INCLUDED__
#define __I_IRRKLANG_AUDIO_STREAM_LOADER_H_INCLUDED__

#include "ik_IRefCounted.h"
#include "ik_IFileReader.h"

namespace irrklang
{

class IAudioStream;


class IAudioStreamLoader : public IRefCounted
{
public:

	
	virtual ~IAudioStreamLoader() {};

	
	
	virtual bool isALoadableFileExtension(const ik_c8* fileName) = 0;

	
	
	virtual IAudioStream* createAudioStream(IFileReader* file) = 0;
};


} 

#endif

