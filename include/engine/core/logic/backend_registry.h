#pragma once

#include <core/type/audio_backend.h>
#include <core/type/graphics_backend.h>
#include <core/type/physics_backend.h>
#include <string_view>

#ifndef AXIS_HAS_OPENGL_BACKEND
#define AXIS_HAS_OPENGL_BACKEND 0
#endif
#ifndef AXIS_HAS_VULKAN_BACKEND
#define AXIS_HAS_VULKAN_BACKEND 0
#endif
#ifndef AXIS_HAS_DIRECTX_BACKEND
#define AXIS_HAS_DIRECTX_BACKEND 0
#endif
#ifndef AXIS_HAS_BULLET_BACKEND
#define AXIS_HAS_BULLET_BACKEND 0
#endif
#ifndef AXIS_HAS_PHYSX_BACKEND
#define AXIS_HAS_PHYSX_BACKEND 0
#endif
#ifndef AXIS_HAS_IRRKLANG_BACKEND
#define AXIS_HAS_IRRKLANG_BACKEND 0
#endif
#ifndef AXIS_HAS_FMOD_BACKEND
#define AXIS_HAS_FMOD_BACKEND 0
#endif
#ifndef AXIS_HAS_OPENAL_BACKEND
#define AXIS_HAS_OPENAL_BACKEND 0
#endif
#ifndef AXIS_HAS_NULL_AUDIO_BACKEND
#define AXIS_HAS_NULL_AUDIO_BACKEND 1
#endif

namespace BackendRegistry
{
constexpr std::string_view ToString(GraphicsBackend backend)
{
    switch (backend)
    {
        case GraphicsBackend::OpenGL:
            return "OpenGL";
        case GraphicsBackend::Vulkan:
            return "Vulkan";
        case GraphicsBackend::DirectX:
            return "DirectX";
    }
    return "Unknown Graphics Backend";
}

constexpr std::string_view ToString(PhysicsBackend backend)
{
    switch (backend)
    {
        case PhysicsBackend::Bullet:
            return "Bullet";
        case PhysicsBackend::PhysX:
            return "PhysX";
    }
    return "Unknown Physics Backend";
}

constexpr std::string_view ToString(AudioBackend backend)
{
    switch (backend)
    {
        case AudioBackend::Null:
            return "Null";
        case AudioBackend::IrrKlang:
            return "IrrKlang";
        case AudioBackend::FMOD:
            return "FMOD";
        case AudioBackend::OpenAL:
            return "OpenAL";
    }
    return "Unknown Audio Backend";
}

constexpr bool IsSupported(GraphicsBackend backend)
{
    switch (backend)
    {
        case GraphicsBackend::OpenGL:
            return AXIS_HAS_OPENGL_BACKEND != 0;
        case GraphicsBackend::Vulkan:
            return AXIS_HAS_VULKAN_BACKEND != 0;
        case GraphicsBackend::DirectX:
            return AXIS_HAS_DIRECTX_BACKEND != 0;
    }
    return false;
}

constexpr bool IsSupported(PhysicsBackend backend)
{
    switch (backend)
    {
        case PhysicsBackend::Bullet:
            return AXIS_HAS_BULLET_BACKEND != 0;
        case PhysicsBackend::PhysX:
            return AXIS_HAS_PHYSX_BACKEND != 0;
    }
    return false;
}

constexpr bool IsSupported(AudioBackend backend)
{
    switch (backend)
    {
        case AudioBackend::Null:
            return AXIS_HAS_NULL_AUDIO_BACKEND != 0;
        case AudioBackend::IrrKlang:
            return AXIS_HAS_IRRKLANG_BACKEND != 0;
        case AudioBackend::FMOD:
            return AXIS_HAS_FMOD_BACKEND != 0;
        case AudioBackend::OpenAL:
            return AXIS_HAS_OPENAL_BACKEND != 0;
    }
    return false;
}

inline const char* SupportedGraphicsBackends()
{
#if AXIS_HAS_OPENGL_BACKEND && !AXIS_HAS_VULKAN_BACKEND && !AXIS_HAS_DIRECTX_BACKEND
    return "OpenGL";
#else
    return "the graphics backends compiled into this build";
#endif
}

inline const char* SupportedPhysicsBackends()
{
#if AXIS_HAS_BULLET_BACKEND && !AXIS_HAS_PHYSX_BACKEND
    return "Bullet";
#else
    return "the physics backends compiled into this build";
#endif
}

inline const char* SupportedAudioBackends()
{
#if AXIS_HAS_IRRKLANG_BACKEND && !AXIS_HAS_FMOD_BACKEND && !AXIS_HAS_OPENAL_BACKEND
    return "IrrKlang";
#elif AXIS_HAS_NULL_AUDIO_BACKEND && !AXIS_HAS_IRRKLANG_BACKEND && !AXIS_HAS_FMOD_BACKEND && !AXIS_HAS_OPENAL_BACKEND
    return "Null";
#else
    return "the audio backends compiled into this build";
#endif
}
}  // namespace BackendRegistry
