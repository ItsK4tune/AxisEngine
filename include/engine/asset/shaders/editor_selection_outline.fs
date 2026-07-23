#version 460 core

layout(location = 0) out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D u_DepthTexture;
uniform usampler2D u_EntityIdTexture;
layout(std430, binding = 29) readonly buffer EditorSelectionData {
    uint u_SelectedEntityIDs[];
};
uniform int u_SelectedEntityCount;
uniform bool u_HasEntityIdTexture;
uniform bool u_HasDepthTexture;

const uint BACKGROUND_ENTITY_ID = 0xffffffffu;

uint ReadEntityId(ivec2 pixel, ivec2 size)
{
    pixel = clamp(pixel, ivec2(0), size - ivec2(1));
    return texelFetch(u_EntityIdTexture, pixel, 0).r;
}

float ReadDepth(ivec2 pixel, ivec2 size)
{
    pixel = clamp(pixel, ivec2(0), size - ivec2(1));
    vec2 uv = (vec2(pixel) + vec2(0.5)) / vec2(size);
    return texture(u_DepthTexture, uv).r;
}

bool IsSelectedId(uint entityId)
{
    // The CPU keeps the list sorted. Binary search avoids a linear scan for
    // every outline sample while keeping the selection representation exact.
    int low = 0;
    int high = u_SelectedEntityCount - 1;
    while (low <= high)
    {
        int middle = low + (high - low) / 2;
        uint selectedId = u_SelectedEntityIDs[middle];
        if (selectedId == entityId)
            return true;
        if (selectedId < entityId)
            low = middle + 1;
        else
            high = middle - 1;
    }
    return false;
}

void main()
{
    vec4 sceneColor = texture(screenTexture, TexCoords);
    if (!u_HasEntityIdTexture || u_SelectedEntityCount == 0)
    {
        FragColor = sceneColor;
        return;
    }

    ivec2 size = textureSize(u_EntityIdTexture, 0);
    ivec2 pixel = clamp(ivec2(TexCoords * vec2(size)), ivec2(0), size - ivec2(1));
    uint centerId = ReadEntityId(pixel, size);
    bool centerSelected = IsSelectedId(centerId);

    // Never paint a pixel owned by an unselected entity. Selected foreground
    // geometry uses an inner edge instead.
    if (!centerSelected && centerId != BACKGROUND_ENTITY_ID)
    {
        FragColor = sceneColor;
        return;
    }

    const ivec2 offsets[8] = ivec2[8](
        ivec2(1, 0), ivec2(-1, 0), ivec2(0, 1), ivec2(0, -1),
        ivec2(1, 1), ivec2(-1, 1), ivec2(1, -1), ivec2(-1, -1));
    bool edge = false;
    float centerDepth = u_HasDepthTexture ? ReadDepth(pixel, size) : 1.0;
    for (int index = 0; index < 8; ++index)
    {
        ivec2 neighborPixel = pixel + offsets[index];
        uint neighborId = ReadEntityId(neighborPixel, size);
        if (neighborId == centerId)
            continue;

        bool neighborSelected = IsSelectedId(neighborId);
        if (!centerSelected)
        {
            if (neighborSelected)
            {
                edge = true;
                break;
            }
            continue;
        }

        // Preserve an individual border between two selected entities instead
        // of merging the complete multi-selection into one silhouette.
        if (neighborSelected || neighborId == BACKGROUND_ENTITY_ID)
        {
            edge = true;
            break;
        }

        // At an occlusion boundary, only outline the selected surface when it
        // is in front. This prevents a selected plane from tracing every
        // unselected object that happens to stand in front of it.
        if (u_HasDepthTexture)
        {
            float neighborDepth = ReadDepth(neighborPixel, size);
            if (centerDepth + 0.00001 < neighborDepth)
            {
                edge = true;
                break;
            }
        }
    }

    FragColor = edge ? vec4(1.0) : sceneColor;
}
