#version 430 core

layout (local_size_x = 16, local_size_y = 16) in;

// 1. UBO Bindings (Range 20-22)
layout(std140, binding = 22) uniform GlobalData {
    float u_Time;
    float u_DeltaTime;
    vec2  u_Resolution;
} globalData;

// 2. SSBO / Images
layout(rgba32f, binding = 0) uniform image2D imgOutput;

void main() {
    // ... compute logic ...
}
