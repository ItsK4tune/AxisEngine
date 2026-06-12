#include <core/logic/compiled_backend_registration.h>

namespace axis::backend
{
#if AXIS_HAS_OPENGL_BACKEND
void RegisterOpenGLBackendFactories();
#endif
#if AXIS_HAS_VULKAN_BACKEND
void RegisterVulkanBackendFactories();
#endif
#if AXIS_HAS_DIRECTX_BACKEND
void RegisterDirectXBackendFactories();
#endif
void RegisterNullGraphicsBackendFactories();
#if AXIS_HAS_NULL_AUDIO_BACKEND
void RegisterNullAudioBackendFactories();
#endif
#if AXIS_HAS_IRRKLANG_BACKEND
void RegisterIrrKlangAudioBackendFactories();
#endif
#if AXIS_HAS_FMOD_BACKEND
void RegisterFMODAudioBackendFactories();
#endif
#if AXIS_HAS_BULLET_BACKEND
void RegisterBulletPhysicsBackendFactories();
#endif
#if AXIS_HAS_PHYSX_BACKEND
void RegisterPhysXPhysicsBackendFactories();
#endif
void RegisterGLFWPlatformBackendFactories();
}  // namespace axis::backend

void RegisterCompiledBackendFactories()
{
    static bool registered = false;
    if (registered)
        return;
    registered = true;

    axis::backend::RegisterNullGraphicsBackendFactories();

#if AXIS_HAS_OPENGL_BACKEND
    axis::backend::RegisterOpenGLBackendFactories();
#endif
#if AXIS_HAS_VULKAN_BACKEND
    axis::backend::RegisterVulkanBackendFactories();
#endif
#if AXIS_HAS_DIRECTX_BACKEND
    axis::backend::RegisterDirectXBackendFactories();
#endif
#if AXIS_HAS_NULL_AUDIO_BACKEND
    axis::backend::RegisterNullAudioBackendFactories();
#endif
#if AXIS_HAS_IRRKLANG_BACKEND
    axis::backend::RegisterIrrKlangAudioBackendFactories();
#endif
#if AXIS_HAS_FMOD_BACKEND
    axis::backend::RegisterFMODAudioBackendFactories();
#endif
#if AXIS_HAS_BULLET_BACKEND
    axis::backend::RegisterBulletPhysicsBackendFactories();
#endif
#if AXIS_HAS_PHYSX_BACKEND
    axis::backend::RegisterPhysXPhysicsBackendFactories();
#endif
    axis::backend::RegisterGLFWPlatformBackendFactories();
}
