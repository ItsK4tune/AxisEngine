#pragma once

#include <stb_image.h>
#include <mutex>

namespace StbImageLoader
{
inline std::mutex& GetLoadMutex()
{
    static std::mutex mutex;
    return mutex;
}

inline unsigned char* Load(const char* filename, int* width, int* height, int* components, int requestedComponents,
                           bool flipVertically)
{
    std::lock_guard<std::mutex> lock(GetLoadMutex());
    stbi_set_flip_vertically_on_load(flipVertically ? 1 : 0);
    return stbi_load(filename, width, height, components, requestedComponents);
}

inline unsigned char* LoadFromMemory(const unsigned char* buffer, int length, int* width, int* height, int* components,
                                     int requestedComponents, bool flipVertically)
{
    std::lock_guard<std::mutex> lock(GetLoadMutex());
    stbi_set_flip_vertically_on_load(flipVertically ? 1 : 0);
    return stbi_load_from_memory(buffer, length, width, height, components, requestedComponents);
}

inline void Free(void* data)
{
    stbi_image_free(data);
}
}  // namespace StbImageLoader
