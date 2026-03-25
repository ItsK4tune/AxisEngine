#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aTexCoords;
uniform mat4 model;
layout(std140, binding = 20) uniform CameraData { mat4 projection; mat4 view; vec3 viewPos; } camera;
out vec2 TexCoords;
void main() {
    TexCoords = aTexCoords;
    gl_Position = camera.projection * camera.view * model * vec4(aPos, 1.0);
}
