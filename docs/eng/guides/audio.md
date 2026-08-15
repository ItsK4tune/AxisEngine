# Audio System Guide

> [Tiếng Việt](../../vi/guides/audio.md) | [Audio Capture Guide](audio_capture.md) | [Components Reference](components_reference.md) | [Documentation Index](../INDEX.md)

---

## 1. Introduction

AxisEngine features a multi-backend audio engine supporting **Null** (default), **FMOD**, and **irrKlang**. Accessed via `IAudioService` and `AudioSourceComponent`, the subsystem handles 2D background music, 3D spatialized audio, distance attenuation, and master volume controls.

---

## 2. How to Use

1. **Selecting Audio Backend**: Set `AXIS_AUDIO_BACKEND` in CMake (`Null`, `FMOD`, `IrrKlang`).
2. **Playing 2D Sounds**: Call `ServiceLocator::Get<IAudioService>()->PlaySound("sound.wav", volume)`.
3. **Attaching 3D Spatial Audio**: Add `AudioSourceComponent` to an entity, set `clipPath`, `volume`, `loop`, `is3D = true`, `minDistance`, and `maxDistance`.
4. **Master Volume**: Call `audio->SetMasterVolume(volumePercent)`.

---

## 3. Examples

### 1. Playing 2D Sound Example
```cpp
#include <axis_sdk.h>

void PlayClickEffect() {
    auto audio = ServiceLocator::Get<IAudioService>();
    if (audio) {
        audio->PlaySound("asset://audio/click.wav", 0.8f);
    }
}
```

### 2. Creating 3D Spatial Audio Source Example
```cpp
#include <axis_sdk.h>

void AttachSpatialAudio(Scene& scene, const Vector3& pos) {
    auto emitter = scene.CreateEntity("Audio Emitter");

    auto& transform = emitter.AddComponent<TransformComponent>();
    transform.SetPosition(pos);

    auto& audio = emitter.AddComponent<AudioSourceComponent>();
    audio.clipPath = "audio/ambient_loop.ogg";
    audio.volume = 1.0f;
    audio.loop = true;
    audio.is3D = true;
    audio.minDistance = 2.0f;
    audio.maxDistance = 40.0f;
}
```

---

## 4. API & Configuration Reference

### Audio Backends & `AudioSourceComponent` Parameters Reference

| Setting / Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `AUDIO_ENGINE` | `Enum` | `NULL` | Selected audio backend (`NULL`, `FMOD`, `IRRKLANG`) |
| `VOLUME` | `int` | `100` | Global master volume in percent (0 to 100) |
| `AudioSourceComponent::clipPath` | `string` | `""` | Audio file resource path (`.wav`, `.ogg`, `.mp3`) |
| `AudioSourceComponent::volume` | `float` | `1.0` | Individual clip volume multiplier (`0.0` to `1.0`) |
| `AudioSourceComponent::pitch` | `float` | `1.0` | Playback speed & pitch multiplier |
| `AudioSourceComponent::loop` | `bool` | `false` | Loops audio playback upon completion |
| `AudioSourceComponent::is3D` | `bool` | `true` | Enables 3D spatial position & attenuation |
| `AudioSourceComponent::minDistance`| `float` | `1.0` | Full volume inner radius distance |
| `AudioSourceComponent::maxDistance`| `float` | `50.0` | Max audible cutoff distance |
