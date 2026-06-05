#include "mocks/fake_resources.h"
#include "test_framework.h"
#include "test_support.h"

#include <resource/logic/resource_manager.h>

using axis_test_mocks::FakeAudioEngine;
using axis_test_mocks::FakeTextureManager;

AXIS_TEST_CASE("ResourceManager GetTextureAuto returns existing texture by name")
{
    FakeTextureManager textureManager;
    FakeAudioEngine audioEngine;
    ResourceManager resources;
    resources.Initialize(nullptr, &textureManager, audioEngine);
    unsigned char pixel[] = {255, 255, 255, 255};

    resources.CreateTextureFromData("existingTexture", pixel, 1, 1, 4);
    auto direct = resources.GetTexture("existingTexture");
    auto resolved = resources.GetTextureAuto("existingTexture");

    AXIS_CHECK(direct != nullptr);
    AXIS_CHECK(resolved == direct);
    AXIS_CHECK(textureManager.texImage2DCount > 0);
    resources.Shutdown();
}

AXIS_TEST_CASE("ResourceManager GetModelAuto resolves by existing resource path")
{
    FakeTextureManager textureManager;
    FakeAudioEngine audioEngine;
    ResourceManager resources;
    resources.Initialize(nullptr, &textureManager, audioEngine);
    auto modelPath = axis_test_support::WriteTempFile(
        "rs_triangle.obj",
        "o Triangle\n"
        "v 0 0 0\n"
        "v 1 0 0\n"
        "v 0 1 0\n"
        "f 1 2 3\n");

    resources.LoadModel("triangleModel", modelPath.string(), false);
    auto direct = resources.GetModel("triangleModel");
    auto resolved = resources.GetModelAuto(modelPath.string());

    AXIS_CHECK(direct != nullptr);
    AXIS_CHECK(resolved == direct);
    resources.Shutdown();
}

AXIS_TEST_CASE("ResourceManager AddResourceDefinition replaces duplicate type and name")
{
    ResourceManager resources;
    resources.InitializeHeadless();

    resources.AddResourceDefinition("Texture", "texA", {{"Path", "A.png"}});
    resources.AddResourceDefinition("Texture", "texA", {{"Path", "B.png"}});

    const auto& definitions = resources.GetResourceDefinitions();
    AXIS_CHECK(definitions.size() == 1);
    AXIS_CHECK(definitions[0].type == "Texture");
    AXIS_CHECK(definitions[0].name == "texA");
    AXIS_CHECK(definitions[0].properties.at("Path") == "B.png");
    resources.Shutdown();
}
