# Hướng dẫn Hệ thống Âm thanh (Audio System)

> [English](../../eng/guides/audio.md) | [Thu âm Audio Capture](audio_capture.md) | [Tra cứu Component Reference](components_reference.md) | [Mục lục Tài liệu](../INDEX.md)

---

## 1. Giới thiệu

AxisEngine sở hữu một engine âm thanh đa backend hỗ trợ **Null** (mặc định), **FMOD** và **irrKlang**. Được truy cập qua `IAudioService` và `AudioSourceComponent`, subsystem này xử lý nhạc nền 2D, âm thanh định vị 3D không gian, suy giảm khoảng cách và điều khiển âm lượng tổng master.

---

## 2. Cách dùng

1. **Lựa chọn Backend Âm thanh**: Đặt `AXIS_AUDIO_BACKEND` trong CMake (`Null`, `FMOD`, `IrrKlang`).
2. **Phát Âm thanh 2D**: Gọi `ServiceLocator::Get<IAudioService>()->PlaySound("sound.wav", volume)`.
3. **Gắn Âm thanh Định vị 3D**: Thêm `AudioSourceComponent` vào entity, thiết lập `clipPath`, `volume`, `loop`, `is3D = true`, `minDistance` và `maxDistance`.
4. **Âm lượng Master**: Gọi `audio->SetMasterVolume(volumePercent)`.

---

## 3. Ví dụ

### 1. Ví dụ Phát Âm thanh 2D
```cpp
#include <axis_sdk.h>

void PlayClickEffect() {
    auto audio = ServiceLocator::Get<IAudioService>();
    if (audio) {
        audio->PlaySound("asset://audio/click.wav", 0.8f);
    }
}
```

### 2. Ví dụ Tạo Nguồn Phát Âm thanh Định vị 3D
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

## 4. Tra cứu Param, Setting & API Reference

### Bảng Tra cứu Backends Âm thanh & Tham số `AudioSourceComponent`

| Tham số / Thuộc tính | Kiểu dữ liệu | Mặc định | Mô tả |
| :--- | :--- | :--- | :--- |
| `AUDIO_ENGINE` | `Enum` | `NULL` | Backend âm thanh được chọn (`NULL`, `FMOD`, `IRRKLANG`) |
| `VOLUME` | `int` | `100` | Âm lượng tổng master tính theo phần trăm (0 đến 100) |
| `AudioSourceComponent::clipPath` | `string` | `""` | Đường dẫn file âm thanh (`.wav`, `.ogg`, `.mp3`) |
| `AudioSourceComponent::volume` | `float` | `1.0` | Hệ số âm lượng clip cá nhân (`0.0` đến `1.0`) |
| `AudioSourceComponent::pitch` | `float` | `1.0` | Hệ số tốc độ phát và cao độ |
| `AudioSourceComponent::loop` | `bool` | `false` | Lặp lại phát âm thanh khi kết thúc |
| `AudioSourceComponent::is3D` | `bool` | `true` | Bật vị trí và suy giảm khoảng cách 3D không gian |
| `AudioSourceComponent::minDistance`| `float` | `1.0` | Khoảng cách bán kính trong âm lượng 100% |
| `AudioSourceComponent::maxDistance`| `float` | `50.0` | Khoảng cách cắt âm thanh không thể nghe thấy |
