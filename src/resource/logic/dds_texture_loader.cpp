#include <resource/logic/dds_texture_loader.h>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <limits>

namespace
{
constexpr uint32_t DdsMagic = 0x20534444u;
constexpr uint32_t FourCC(char a, char b, char c, char d)
{
    return static_cast<uint32_t>(a) | (static_cast<uint32_t>(b) << 8) |
           (static_cast<uint32_t>(c) << 16) | (static_cast<uint32_t>(d) << 24);
}

#pragma pack(push, 1)
struct DdsPixelFormat
{
    uint32_t size;
    uint32_t flags;
    uint32_t fourCC;
    uint32_t rgbBitCount;
    uint32_t rMask;
    uint32_t gMask;
    uint32_t bMask;
    uint32_t aMask;
};

struct DdsHeader
{
    uint32_t size;
    uint32_t flags;
    uint32_t height;
    uint32_t width;
    uint32_t pitchOrLinearSize;
    uint32_t depth;
    uint32_t mipMapCount;
    uint32_t reserved[11];
    DdsPixelFormat pixelFormat;
    uint32_t caps;
    uint32_t caps2;
    uint32_t caps3;
    uint32_t caps4;
    uint32_t reserved2;
};

struct DdsHeaderDx10
{
    uint32_t dxgiFormat;
    uint32_t resourceDimension;
    uint32_t miscFlag;
    uint32_t arraySize;
    uint32_t miscFlags2;
};
#pragma pack(pop)

static_assert(sizeof(DdsPixelFormat) == 32);
static_assert(sizeof(DdsHeader) == 124);
static_assert(sizeof(DdsHeaderDx10) == 20);

bool ResolveLegacyFormat(uint32_t fourCC, InternalFormat& format, int& blockBytes, int& components)
{
    switch (fourCC)
    {
        case FourCC('D', 'X', 'T', '1'):
            format = InternalFormat::BC1RGBA; blockBytes = 8; components = 4; return true;
        case FourCC('D', 'X', 'T', '3'):
            format = InternalFormat::BC2RGBA; blockBytes = 16; components = 4; return true;
        case FourCC('D', 'X', 'T', '5'):
            format = InternalFormat::BC3RGBA; blockBytes = 16; components = 4; return true;
        case FourCC('A', 'T', 'I', '1'):
        case FourCC('B', 'C', '4', 'U'):
            format = InternalFormat::BC4R; blockBytes = 8; components = 1; return true;
        case FourCC('A', 'T', 'I', '2'):
        case FourCC('B', 'C', '5', 'U'):
            format = InternalFormat::BC5RG; blockBytes = 16; components = 2; return true;
        default:
            return false;
    }
}

bool ResolveDxgiFormat(uint32_t dxgi, InternalFormat& format, int& blockBytes, int& components)
{
    switch (dxgi)
    {
        case 71: case 72: format = InternalFormat::BC1RGBA; blockBytes = 8; components = 4; return true;
        case 74: case 75: format = InternalFormat::BC2RGBA; blockBytes = 16; components = 4; return true;
        case 77: case 78: format = InternalFormat::BC3RGBA; blockBytes = 16; components = 4; return true;
        case 80: case 81: format = InternalFormat::BC4R; blockBytes = 8; components = 1; return true;
        case 83: case 84: format = InternalFormat::BC5RG; blockBytes = 16; components = 2; return true;
        default: return false;
    }
}

bool Fail(std::string* error, const char* message)
{
    if (error)
        *error = message;
    return false;
}
}  // namespace

bool LoadDdsTexture(const std::string& path, CompressedTextureData& output, std::string* error)
{
    output = {};
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return Fail(error, "cannot open DDS file");

    uint32_t magic = 0;
    DdsHeader header{};
    input.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    input.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (!input || magic != DdsMagic || header.size != sizeof(DdsHeader) ||
        header.pixelFormat.size != sizeof(DdsPixelFormat))
        return Fail(error, "invalid or truncated DDS header");
    if (header.width == 0 || header.height == 0 || header.width > 32768 || header.height > 32768)
        return Fail(error, "DDS dimensions are outside the supported range");
    if ((header.caps2 & 0x00000200u) != 0 || header.depth > 1)
        return Fail(error, "DDS cubemaps and volume textures are not supported by the 2D loader");

    int blockBytes = 0;
    if (header.pixelFormat.fourCC == FourCC('D', 'X', '1', '0'))
    {
        DdsHeaderDx10 dx10{};
        input.read(reinterpret_cast<char*>(&dx10), sizeof(dx10));
        if (!input)
            return Fail(error, "truncated DDS DX10 header");
        if (dx10.resourceDimension != 3 || dx10.arraySize != 1 || (dx10.miscFlag & 0x4u) != 0)
            return Fail(error, "only single 2D DDS DX10 resources are supported");
        if (!ResolveDxgiFormat(dx10.dxgiFormat, output.format, blockBytes, output.components))
            return Fail(error, "DDS DXGI format is not BC1-BC5");
    }
    else if (!ResolveLegacyFormat(header.pixelFormat.fourCC, output.format, blockBytes, output.components))
    {
        return Fail(error, "DDS FourCC is not BC1-BC5");
    }

    output.width = static_cast<int>(header.width);
    output.height = static_cast<int>(header.height);
    const uint32_t mipCount = std::clamp(header.mipMapCount == 0 ? 1u : header.mipMapCount, 1u, 32u);
    output.mips.reserve(mipCount);
    uint32_t width = header.width;
    uint32_t height = header.height;
    for (uint32_t level = 0; level < mipCount; ++level)
    {
        const uint64_t blocksWide = (std::max)(1u, (width + 3u) / 4u);
        const uint64_t blocksHigh = (std::max)(1u, (height + 3u) / 4u);
        const uint64_t byteCount64 = blocksWide * blocksHigh * static_cast<uint64_t>(blockBytes);
        if (byteCount64 > (std::numeric_limits<size_t>::max)())
            return Fail(error, "DDS mip payload is too large");
        CompressedTextureMip mip;
        mip.width = static_cast<int>(width);
        mip.height = static_cast<int>(height);
        mip.bytes.resize(static_cast<size_t>(byteCount64));
        input.read(reinterpret_cast<char*>(mip.bytes.data()), static_cast<std::streamsize>(mip.bytes.size()));
        if (!input)
            return Fail(error, "truncated DDS mip payload");
        output.mips.push_back(std::move(mip));
        width = (std::max)(1u, width / 2u);
        height = (std::max)(1u, height / 2u);
    }
    return true;
}
