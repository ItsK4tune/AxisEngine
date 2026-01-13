# Post Processing Guide

The Post Processing system allows you to apply full-screen effects (shaders) to the rendered scene. Currently, this is configured purely through C++ code, but relies on shaders loaded in the scene.

---

## 1. Setup in Scene File

First, load your post-processing shaders in your `.scene` file.

```text
# LOAD_SHADER <name> <vertex> <fragment>
LOAD_SHADER    InvertEffect    resources/shaders/ppt.vs    resources/shaders/invert.fs
LOAD_SHADER    GrayScale       resources/shaders/ppt.vs    resources/shaders/grayscale.fs
```

> **Note**: Post Process shaders typically use a simple screen-quad vertex shader (often `ppt.vs`) that passes valid texture coordinates.

---

## 2. Applying Effects in Code

You can add effects to the pipeline in your Game State (e.g., `GameState::Init`) or Script.

```cpp
#include <engine/engine.h>

void GameState::Init()
{
    // Retrieve the shader loaded from the scene
    Shader* invertShader = m_App->GetResources().GetShader("InvertEffect");
    
    // Add to PostProcess Pipeline
    // AddEffect(Shader* shader, float x, float y, float width, float height)
    if (invertShader)
    {
        // 1.0f = full screen relative size
        m_App->GetPostProcess().AddEffect(invertShader, 0, 0, 1.0f, 1.0f); 
    }
}
```

---

## 3. Pipeline Order
Effects are applied in the order they are added. The output of one effect becomes the input texture of the next (Ping-Pong buffering).

---

## 4. API Reference

### `PostProcessPipeline`

#### `AddEffect(Shader* shader, float x, float y, float w, float h)`
*   **shader**: The shader program to use.
*   **x, y**: Viewport offset (0.0 - 1.0).
*   **w, h**: Viewport size (0.0 - 1.0). Default is full screen (1.0).
