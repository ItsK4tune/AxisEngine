#version 460 core

layout(location = 0) out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform usampler2D u_EntityIdTexture;
uniform uint u_SelectedEntityID;
uniform bool u_HasEntityIdTexture;

bool IsSelected(ivec2 pixel, ivec2 size)
{
    pixel = clamp(pixel, ivec2(0), size - ivec2(1));
    return texelFetch(u_EntityIdTexture, pixel, 0).r == u_SelectedEntityID;
}

void main()
{
    vec4 sceneColor = texture(screenTexture, TexCoords);
    if (!u_HasEntityIdTexture || u_SelectedEntityID == 0xFFFFFFFFu)
    {
        FragColor = sceneColor;
        return;
    }

    ivec2 size = textureSize(u_EntityIdTexture, 0);
    ivec2 pixel = clamp(ivec2(TexCoords * vec2(size)), ivec2(0), size - ivec2(1));
    bool center = IsSelected(pixel, size);
    bool neighbor = IsSelected(pixel + ivec2(1, 0), size) ||
                    IsSelected(pixel + ivec2(-1, 0), size) ||
                    IsSelected(pixel + ivec2(0, 1), size) ||
                    IsSelected(pixel + ivec2(0, -1), size) ||
                    IsSelected(pixel + ivec2(1, 1), size) ||
                    IsSelected(pixel + ivec2(-1, 1), size) ||
                    IsSelected(pixel + ivec2(1, -1), size) ||
                    IsSelected(pixel + ivec2(-1, -1), size);

    FragColor = (!center && neighbor) ? vec4(1.0) : sceneColor;
}
