# Thu âm microphone

> [English](../../eng/guides/audio_capture.md)

Thu âm độc lập với playback. AxisEngine đăng ký `IAudioCaptureService`; Windows
dùng WASAPI shared mode, nền tảng khác trả unsupported service cho đến khi ứng
dụng cung cấp backend.

## Cấu hình

```yaml
axis_config:
  AUDIO_CAPTURE_ENABLED: 1
  AUDIO_CAPTURE_DEVICE: ""          # rỗng: endpoint mặc định
  AUDIO_CAPTURE_INPUT_VOLUME: 1.0   # pre-amplifier phần mềm, 0.0-4.0
  AUDIO_CAPTURE_NOISE_GATE: 0.02
  AUDIO_CAPTURE_GAIN: 4.0
  AUDIO_CAPTURE_ATTACK_SECONDS: 0.05
  AUDIO_CAPTURE_RELEASE_SECONDS: 0.05
  AUDIO_CAPTURE_PEAK_DECAY_SECONDS: 0.125
  AUDIO_CAPTURE_CALIBRATION_SECONDS: 1.0
  AUDIO_CAPTURE_PULSE_THRESHOLD: 0.15
  AUDIO_CAPTURE_PULSE_COOLDOWN: 0.08
  AUDIO_CAPTURE_PULSE_DURATION: 0.6
```

`MIC_INPUT_VOLUME` và `MIC_INPUT_THRESHOLD` là alias cũ. Input volume áp trước
calibration/noise gate; gain áp sau khi chuẩn hóa. Attack/release điều khiển RMS,
peak decay giữ transient. `GetSnapshot()` trả bản sao thread-safe gồm RMS, peak,
intensity và pulse có giới hạn.

## Thay backend

Implement `IAudioCaptureService`, cài bằng
`AppBuilder::WithAudioCaptureFactory`. Backend có thể đưa RMS/peak thô vào
`AudioCaptureProcessor` để dùng chung sanitization, smoothing, calibration,
noise gate và pulse lifecycle. Service sở hữu phải tự đồng bộ capture thread
với frame thread.

Custom post-process đọc level qua `u_AudioLevel` và pulse qua SSBO binding `26`;
xem [Graphics](graphics.md#abi-custom-post-process).
