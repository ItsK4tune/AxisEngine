# Hướng dẫn giao diện người dùng

> [English](../../eng/guides/ui.md)

AxisEngine có overlay UI 2D responsive cho menu, HUD và phần tử tương tác.

## 1. Component

### UITransformComponent

`anchorMin`/`anchorMax` là tọa độ chuẩn hóa trong parent; `offsetMin`/
`offsetMax` là pixel offset; `pivot` là tâm scale/rotate; `ZOrder` quyết định lớp vẽ.

### UIRendererComponent

Vẽ quad màu/texture, hỗ trợ tint và custom UI shader.

### UITextComponent

Text động với font preload, alignment `Left`/`Center`/`Right`; bật `wordWrap`
và đặt `maxWidth` để giới hạn dòng.

### UIFlexLayoutComponent

Sắp child theo `Row` hoặc `Column`, có spacing và padding bốn cạnh.

## 2. Tương tác và animation

`UIInteractiveComponent` có state `hovered`, `pressed`, `clicked`, `holdTime`
và callback runtime `onClick`, `onHoverEnter`, `onReleased`. Callback không
được serialize vào `.axs`.

`UIAnimationComponent` blend normal/hover color và dùng `visualScale`; không
sửa trực tiếp size do layout sở hữu.

## 3. Layout responsive

- `UsePercentage`: kích thước theo viewport.
- Anchor giữ phần tử ở cạnh/tâm khi đổi aspect ratio.
- `UI_REFERENCE_WIDTH`, `UI_REFERENCE_HEIGHT` hoặc `UI_REFERENCE_SIZE` đặt
  reference canvas dùng chung cho render và hit test.

## 4. System

1. `UIInteractSystem` xử lý pointer/click và callback.
2. `UIRenderSystem` vẽ late pass trên scene 3D.

Input do script xử lý vẫn dùng cùng reference canvas.

## Xem thêm

- [Graphics](graphics.md)
- [Scriptable API](../scripting/scriptable_api.md)
- [Scene format](scene_format.md)
