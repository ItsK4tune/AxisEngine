#include "test_framework.h"
#include "test_support.h"
#include <resource/logic/dds_texture_loader.h>
#include <cstdint>
#include <fstream>
#include <vector>

namespace
{
void WriteU32(std::vector<uint8_t>& bytes, size_t offset, uint32_t value)
{
    bytes[offset + 0] = static_cast<uint8_t>(value & 0xff);
    bytes[offset + 1] = static_cast<uint8_t>((value >> 8) & 0xff);
    bytes[offset + 2] = static_cast<uint8_t>((value >> 16) & 0xff);
    bytes[offset + 3] = static_cast<uint8_t>((value >> 24) & 0xff);
}

std::vector<uint8_t> MakeBc1Dds(bool includePayload)
{
    std::vector<uint8_t> bytes(128 + (includePayload ? 8 : 0), 0);
    WriteU32(bytes, 0, 0x20534444u);       // DDS magic
    WriteU32(bytes, 4, 124);               // header size
    WriteU32(bytes, 12, 4);                // height
    WriteU32(bytes, 16, 4);                // width
    WriteU32(bytes, 28, 1);                // mip count
    WriteU32(bytes, 76, 32);               // pixel-format size
    WriteU32(bytes, 80, 0x4);              // DDPF_FOURCC
    WriteU32(bytes, 84, 0x31545844u);       // DXT1
    return bytes;
}
}  // namespace

AXIS_TEST_CASE("DDS loader accepts bounded BC1 mip payloads")
{
    const auto path = axis_test_support::TempPath("bc1_texture.dds");
    const auto bytes = MakeBc1Dds(true);
    std::ofstream output(path, std::ios::binary);
    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    output.close();

    CompressedTextureData texture;
    std::string error;
    AXIS_CHECK(LoadDdsTexture(path.string(), texture, &error));
    AXIS_CHECK(texture.width == 4);
    AXIS_CHECK(texture.height == 4);
    AXIS_CHECK(texture.format == InternalFormat::BC1RGBA);
    AXIS_CHECK(texture.mips.size() == 1);
    AXIS_CHECK(texture.mips.front().bytes.size() == 8);
}

AXIS_TEST_CASE("DDS loader rejects a truncated BC payload without partial output")
{
    const auto path = axis_test_support::TempPath("truncated_texture.dds");
    const auto bytes = MakeBc1Dds(false);
    std::ofstream output(path, std::ios::binary);
    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    output.close();

    CompressedTextureData texture;
    std::string error;
    AXIS_CHECK(!LoadDdsTexture(path.string(), texture, &error));
    AXIS_CHECK(!error.empty());
}
