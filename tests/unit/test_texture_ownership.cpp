#include "mocks/fake_resources.h"
#include "test_framework.h"

#include <render/type/graphics_types.h>
#include <resource/logic/texture_manager.h>
#include <vector>

AXIS_TEST_CASE("Texture pixel data ownership survives value copies")
{
    unsigned char pixels[] = {1, 2, 3, 4};
    Texture texture;
    texture.SetPixelDataCopy(pixels, sizeof(pixels));

    Texture copy = texture;
    texture.ReleasePixelData();

    AXIS_CHECK(texture.pixelData == nullptr);
    AXIS_CHECK(!texture.OwnsPixelData());
    AXIS_CHECK(copy.pixelData != nullptr);
    AXIS_CHECK(copy.OwnsPixelData());
    AXIS_CHECK(copy.pixelData[0] == 1);

    copy.ReleasePixelData();
    AXIS_CHECK(copy.pixelData == nullptr);
}

AXIS_TEST_CASE("Texture pixel data ownership survives vector reallocation")
{
    std::vector<Texture> textures;

    for (int i = 0; i < 2; ++i)
    {
        unsigned char pixels[] = {static_cast<unsigned char>(i), 2, 3, 4};
        Texture texture;
        texture.SetPixelDataCopy(pixels, sizeof(pixels));
        textures.push_back(texture);
    }

    AXIS_CHECK(textures.size() == 2);
    AXIS_CHECK(textures[0].pixelData != nullptr);
    AXIS_CHECK(textures[1].pixelData != nullptr);
    AXIS_CHECK(textures[0].OwnsPixelData());
    AXIS_CHECK(textures[1].OwnsPixelData());
}

AXIS_TEST_CASE("TextureManager unload releases kept CPU pixel data")
{
    axis_test_mocks::FakeTextureManager lowLevel;
    TextureManager textures(lowLevel);
    unsigned char pixels[] = {255, 0, 0, 255};

    auto texture = textures.CreateFromData("cpu_texture", pixels, 1, 1, 4, true);
    AXIS_CHECK(texture != nullptr);
    AXIS_CHECK(texture->pixelData != nullptr);
    AXIS_CHECK(texture->OwnsPixelData());

    textures.Unload("cpu_texture");

    AXIS_CHECK(texture->pixelData == nullptr);
    AXIS_CHECK(!texture->OwnsPixelData());
    AXIS_CHECK(lowLevel.deletedTextureCount == 1);
}
