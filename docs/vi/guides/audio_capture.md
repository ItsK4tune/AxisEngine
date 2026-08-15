# Hướng dẫn Thu âm Audio Capture (WASAPI Microphone)

> [English](../../eng/guides/audio_capture.md) | [Hệ thống Âm thanh](audio.md) | [Tra cứu Cấu hình Configuration](configuration.md) | [Mục lục Tài liệu](../INDEX.md)

---

## 1. Giới thiệu

AxisEngine tích hợp sẵn tính năng **Thu âm Microphone WASAPI** native trên hệ thống Windows. Được quản lý qua `IAudioCaptureService`, subsystem thu âm xử lý nạp luồng micro thời gian thực, áp dụng khuếch đại trước gain, lọc ngưỡng tiếng ồn (noise gate), hiệu chỉnh tiếng ồn môi trường và phát hiện xung âm thanh phục vụ cơ chế game phản hồi giọng nói.

---

## 2. Cách dùng

1. **Bật Thu âm Audio Capture**: Đặt `AUDIO_CAPTURE_ENABLED: 1` trong `axis_config.axs`.
2. **Truy cập Dịch vụ**: Lấy instance dịch vụ qua `ServiceLocator::Get<IAudioCaptureService>()`.
3. **Truy vấn Mức Âm lượng**: Gọi `capture->GetNormalizedLevel()` (trả về `0.0` đến `1.0`).
4. **Đăng ký Sự kiện**: Đăng ký hàm lắng nghe sự kiện `VoicePulseEvent` qua `EventManager`.

---

## 3. Ví dụ

### 1. Ví dụ Theo dõi Mức Âm lượng Microphone
```cpp
#include <axis_sdk.h>

void UpdateMicVolumeMeter() {
    auto capture = ServiceLocator::Get<IAudioCaptureService>();
    if (capture && capture->IsCapturing()) {
        float level = capture->GetNormalizedLevel();
        if (level > 0.6f) {
            AXIS_LOG_INFO("Phat hien dinh am thanh giong noi: " + std::to_string(level));
        }
    }
}
```

### 2. Ví dụ Đăng ký Sự kiện Kích hoạt Xung Giọng nói
```cpp
#include <axis_sdk.h>

struct VoicePulseEvent { float peakAmplitude; };

void SetupVoiceSpells() {
    EventManager::Get().Subscribe<VoicePulseEvent>([](const VoicePulseEvent& e) {
        AXIS_LOG_INFO("Phep thuat giong noi duoc kich hoat voi bien do: " + std::to_string(e.peakAmplitude));
    });
}
```

---

## 4. Tra cứu Param, Setting & API Reference

### Bảng Tra cứu Thiết lập Audio Capture

| Khóa Cấu hình | Kiểu dữ liệu | Mặc định | Mô tả |
| :--- | :--- | :--- | :--- |
| `AUDIO_CAPTURE_ENABLED` | `bool` | `0` | Bật khởi tạo thiết bị thu âm microphone |
| `AUDIO_CAPTURE_DEVICE` | `string` | `""` | ID thiết bị cụ thể (để trống dùng mặc định hệ thống) |
| `AUDIO_CAPTURE_INPUT_VOLUME`| `float` | `1.0` | Tỷ lệ khuếch đại trước đầu vào |
| `AUDIO_CAPTURE_NOISE_GATE` | `float` | `0.05` | Ngưỡng biên độ tối thiểu cắt bớt tín hiệu tiếng ồn |
| `AUDIO_CAPTURE_GAIN` | `float` | `1.0` | Hệ số khuếch đại đầu ra sau lọc |
| `AUDIO_CAPTURE_PULSE_THRESHOLD`| `float` | `0.4` | Ngưỡng biên độ đỉnh kích hoạt sự kiện xung giọng nói |
