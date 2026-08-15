# Hướng dẫn Serialization Scene (`.axs` & `.axsb`)

> [English](../../eng/guides/scene_format.md) | [Tra cứu Component Reference](components_reference.md) | [Tra cứu Cấu hình Configuration](configuration.md) | [Mục lục Tài liệu](../INDEX.md)

---

## 1. Giới thiệu

AxisEngine hỗ trợ hai định dạng file serialization chính: dạng văn bản dễ đọc `.axs` (tập con của YAML) và dạng nhị phân biên dịch sẵn `.axsb`. File `.axs` cho phép các nhà phát triển tạo scene, gán nút input, bảng dữ liệu, chuỗi đa ngôn ngữ và cấu hình engine bằng văn bản thuần, trong khi file `.axsb` tối ưu hóa tốc độ nạp màn chơi cho bản phát hành.

---

## 2. Cách dùng

1. **Soạn thảo File `.axs`**: Tạo file văn bản sử dụng khoảng trắng để thụt đầu dòng (nghiêm cấm dùng phím tab) và chỉ định 1 trong 5 khóa root schema (`axis_scene`, `axis_input`, `axis_data`, `axis_localization`, hoặc `axis_config`).
2. **Biên dịch sang `.axsb`**: Chạy `axis_compile input.axs output.axsb` qua dòng lệnh CLI.
3. **Nạp trong C++**: Gọi `SceneManager::Get().LoadScene("scene.axs")` hoặc `LoadBinaryScene("scene.axsb")`.

---

## 3. Ví dụ

### 1. Ví dụ Schema `axis_scene`
```yaml
axis_scene:
    Version: 1.0

Resources:
    Textures:
        - Name: "crate_diffuse"
          Path: "textures/crate_d.png"

Entities:
    - Name: "Sun Light"
      Transform:
          Position: 0.0 10.0 0.0
          Rotation: 45.0 -30.0 0.0
      DirectionalLight:
          Color: 1.0 0.95 0.8
          Intensity: 2.5

    - Name: "Box Entity"
      Transform:
          Position: 0.0 1.0 0.0
      MeshRenderer:
          Model: "models/cube.obj"
          AlbedoTexture: "crate_diffuse"
```

### 2. Ví dụ Schema `axis_input`
```yaml
axis_input:
    Bindings:
        MoveForward:
            Key: W
            GamepadAxis: LeftY
        Jump:
            Key: Space
            GamepadButton: South
```

### 3. Ví dụ Schema `axis_data`
```yaml
axis_data:
    PlayerStats:
        BaseHealth: 100
        BaseSpeed: 5.5
        MaxInventorySlots: 20
```

### 4. Ví dụ Schema `axis_localization`
```yaml
axis_localization:
    Language: "vi_VN"
    Strings:
        UI_PLAY: "CHƠI NGAY"
        UI_QUIT: "THOÁT GAME"
```

---

## 4. Tra cứu Param, Setting & API Reference

### Bảng Tra cứu Chi tiết 5 Loại Schema File `.axs`

| Khóa Root Schema | Mục đích Chính | Các Mục Con Chính | Subsystem Nạp / Phân tích |
| :--- | :--- | :--- | :--- |
| `axis_scene` | Phân cấp scene, entity, tài nguyên và ánh sáng môi trường | `Resources`, `Environment`, `Entities` | `SceneManager`, `YAMLParser` |
| `axis_input` | Gán phím hành động, ánh xạ phím, trục chuột và gamepad | `Bindings`, `Actions`, `Axes` | `InputSerializer`, `InputManager` |
| `axis_data` | Bảng dữ liệu key-value, chỉ số vũ khí, tham số cân bằng | Các node dữ liệu tùy chỉnh | `DataLoader`, `YAMLParser` |
| `axis_localization` | Từ điển dịch thuật đa ngôn ngữ và chuỗi văn bản UI | `Language`, `Strings` | `LocalizationService` |
| `axis_config` | Thiết lập toàn cục engine, cửa sổ, đồ họa và vật lý | Tham số Graphics, Physics, Audio | `ConfigManager`, `Application` |

### Bảng Tra cứu Cú pháp Định dạng Scene & Trình biên dịch

| Quy tắc / Thuộc tính | Ràng buộc | Mô tả |
| :--- | :--- | :--- |
| Thụt đầu dòng | Chỉ dùng Khoảng trắng | Nghiêm cấm dùng phím Tab và sẽ gây ra lỗi cú pháp dòng/cột |
| Định dạng Key-Value | `key: value` | Cặp khóa-giá trị trên một dòng |
| Cú pháp Không hỗ trợ | Anchor (`&`), Alias (`*`), Flow (`[...]`), Chuỗi nhiều dòng (`\|`) | Các tính năng YAML tổng quát bị bỏ qua để tăng tốc độ parse |
| CLI `axis_compile` | `axis_compile <in.axs> <out.axsb>` | File thực thi biên dịch scene nhị phân |
| Rollback Nhị phân | Tự động | Việc nạp file nhị phân lỗi sẽ tự động rollback an toàn |
