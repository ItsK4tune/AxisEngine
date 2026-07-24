# Hướng dẫn audio

> [English](../../eng/guides/audio.md)

Playback đi qua `IAudioEngine`; backend hiện có là Null và tùy chọn irrKlang/FMOD.

## 1. AudioSourceComponent

- `Path`: path tương đối tới audio.
- `Volume`: `0.0`–`1.0`.
- `Loop`: lặp playback.
- `Is3D`: attenuation/panning theo không gian.
- `MinDistance`: bán kính âm lượng tối đa.
- `PlayOnAwake`: phát khi scene load.

## 2. AudioService

```cpp
if (auto* audio = Resolve<AudioService>()) {
    audio->Play2D("audio/music.ogg", true);
}
```

- `Play2D`: nhạc nền/UI.
- `Play3D`: âm thanh tại world position.
- `EmitPulse`: pulse tường minh cho gameplay/post-process.
- `StopAll`, `SetGlobalVolume`: điều khiển toàn cục.
- Listener tự đồng bộ với active camera.

```cpp
if (auto* audio = Resolve<AudioService>()) {
    const glm::vec3 origin = GetComponent<PositionComponent>().value;
    audio->Play3D("audio/footstep.wav", origin);
    audio->EmitPulse(origin, 0.8f, 0.45f);
}
```

Pulse microphone tự phát ở listener; cả hai nguồn dùng buffer GPU tối đa 64 pulse.

## 3. Lifecycle

Mỗi frame `AudioSystem`:

1. Đồng bộ source 3D với transform.
2. Đồng bộ listener với camera.
3. Khởi động source `PlayOnAwake` hoặc được logic yêu cầu.
4. Giải phóng instance backend khi entity bị hủy.

## Xem thêm

- [Physics](physics.md)
- [Graphics](graphics.md)
- [Scriptable API](../scripting/scriptable_api.md)
