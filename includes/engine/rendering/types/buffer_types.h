#pragma once

namespace Graphics {

    enum class BufferType {
        ArrayBuffer,
        ElementArrayBuffer,
        UniformBuffer,
        ShaderStorageBuffer
    };

    enum class BufferUsage {
        StreamDraw,
        StreamRead,
        StreamCopy,
        StaticDraw,
        StaticRead,
        StaticCopy,
        DynamicDraw,
        DynamicRead,
        DynamicCopy
    };
}
