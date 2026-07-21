#pragma once

#include <render/type/graphics_types.h>
#include <cstdint>
#include <string>
#include <vector>

struct CompressedTextureMip
{
    int width = 0;
    int height = 0;
    std::vector<uint8_t> bytes;
};

struct CompressedTextureData
{
    int width = 0;
    int height = 0;
    int components = 4;
    InternalFormat format = InternalFormat::BC1RGBA;
    std::vector<CompressedTextureMip> mips;
};

// Loads 2D DDS textures containing BC1/BC2/BC3/BC4/BC5 blocks, including
// legacy FourCC and DX10 headers. Cubemaps, arrays and uncompressed DDS files
// are rejected explicitly instead of being interpreted as raw pixels.
bool LoadDdsTexture(const std::string& path, CompressedTextureData& output, std::string* error = nullptr);
