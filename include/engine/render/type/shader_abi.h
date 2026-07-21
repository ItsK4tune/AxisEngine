#pragma once

#include <cstdint>

namespace ShaderABI
{
// Keep these values synchronized with the built-in GLSL declarations. Custom
// shaders should include the generated documentation contract rather than
// inventing binding slots locally.
inline constexpr int MaxBones = 128;
inline constexpr int CustomPortCount = 8;
inline constexpr int MaxAudioPulses = 64;

inline constexpr uint32_t CameraUBOBinding = 20;
inline constexpr uint32_t LightUBOBinding = 21;
inline constexpr uint32_t GlobalUBOBinding = 22;

inline constexpr uint32_t DirectionalLightSSBOBinding = 23;
inline constexpr uint32_t PointLightSSBOBinding = 24;
inline constexpr uint32_t SpotLightSSBOBinding = 25;
inline constexpr uint32_t PulseSSBOBinding = 26;
inline constexpr uint32_t LightTileGridSSBOBinding = 27;
inline constexpr uint32_t LightTileIndicesSSBOBinding = 28;

inline constexpr int MaterialAlbedoTexture = 0;
inline constexpr int MaterialNormalTexture = 1;
inline constexpr int MaterialMetallicTexture = 2;
inline constexpr int MaterialRoughnessTexture = 3;
inline constexpr int MaterialAOTexture = 4;
inline constexpr int MaterialEmissiveTexture = 5;
inline constexpr int IrradianceTexture = 6;
inline constexpr int PrefilterTexture = 7;
inline constexpr int BrdfLUTTexture = 8;
inline constexpr int MaterialSpecularTexture = 9;

inline constexpr int PostProcessColorTexture = 0;
inline constexpr int PostProcessDepthTexture = 1;
inline constexpr int PostProcessNormalTexture = 2;
inline constexpr int PostProcessWorldPositionTexture = 3;
}  // namespace ShaderABI
