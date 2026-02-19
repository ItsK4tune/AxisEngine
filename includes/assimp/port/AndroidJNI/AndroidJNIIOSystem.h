



#if __ANDROID__ and __ANDROID_API__ > 9 and defined(AI_CONFIG_ANDROID_JNI_ASSIMP_MANAGER_SUPPORT)
#ifndef AI_ANDROIDJNIIOSYSTEM_H_INC
#define AI_ANDROIDJNIIOSYSTEM_H_INC

#include <assimp/DefaultIOSystem.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/native_activity.h>

namespace Assimp	{



class ASSIMP_API AndroidJNIIOSystem : public DefaultIOSystem {
public:
	
	std::string mApkWorkspacePath;
	AAssetManager* mApkAssetManager;

	
	AndroidJNIIOSystem(ANativeActivity* activity);

    
	AndroidJNIIOSystem(const char *internalPath, AAssetManager* assetManager);

	
	~AndroidJNIIOSystem();

	
	bool Exists( const char* pFile) const;

	
	IOStream* Open( const char* strFile, const char* strMode);

	
	void AndroidActivityInit(ANativeActivity* activity);

	
	bool AndroidExtractAsset(std::string name);
};

} 

#endif 
#endif 
