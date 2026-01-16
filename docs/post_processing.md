# Post Processing Guide

The AXIS Engine includes a post-processing stack that applies effects to the final rendered image before displaying it to the screen.

## 1. How it Works
1.  The scene is rendered to an off-screen Framebuffer (FBO).
2.  The FBO texture is drawn onto a full-screen quad.
3.  A Post-Processing Shader is applied to this quad.

## 2. Default Pipeline
The engine comes with a default post-processing pipeline (`PostProcessPipeline`) that supports:
- **Inversion**: Inverts colors.
- **Grayscale**: Converts to black and white.
- **Sharpen**: Applies a sharpening kernel.
- **Blur**: Applies a blur kernel.
- **Edge Detection**: Highlights edges.

## 3. Configuration
You can control post-processing effects via the `PostProcessPipeline` class.

```cpp
auto& postProcess = m_App->GetPostProcess();

// Enable Grayscale
postProcess.SetEffect(PostProcessEffect::GRAYSCALE);

// Disable effects
postProcess.SetEffect(PostProcessEffect::NONE);
```

## 4. Custom Shaders
The post-processing shader is located at `resources/shaders/postprocess.fs`. You can modify this file to add custom effects like Bloom, Vignette, or Color Correction.

### Shader Structure
The vertex shader simply renders a full-screen quad. The fragment shader samples the screen texture and applies the effect.

```glsl
// Fragment Shader
in vec2 TexCoords;
out vec4 color;

uniform sampler2D screenTexture;
uniform int effectType; // Controlled by code

void main() {
    // Sample texture
    vec3 col = texture(screenTexture, TexCoords).rgb;
    
    // Apply Logic based on effectType
    if (effectType == 1) { // Inversion
        color = vec4(1.0 - col, 1.0);
    } 
    // ...
}
```
