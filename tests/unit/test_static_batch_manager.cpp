#include "test_framework.h"
#include "test_support.h"

#include <render/logic/static_batch_manager.h>
#include <cstdint>
#include <fstream>
#include <vector>

AXIS_TEST_CASE("StaticBatchManager loads and saves portable v2 batch files")
{
    auto path = axis_test_support::TempPath("portable_static_batch.btch");
    {
        std::ofstream os(path, std::ios::binary);
        auto writeU32 = [&](uint32_t value) { os.write(reinterpret_cast<const char*>(&value), sizeof(value)); };
        auto writeF32 = [&](float value) { os.write(reinterpret_cast<const char*>(&value), sizeof(value)); };

        writeU32(0x48435442);
        writeU32(2);
        writeU32(1);
        writeU32(3);

        for (float value : {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 0.25f, 0.75f})
        {
            writeF32(value);
        }

        writeU32(0);
        writeU32(0);
        writeU32(0);
    }

    StaticBatchManager manager;
    AXIS_CHECK(manager.LoadBatchFromFile("portable", path.string()));
    AXIS_CHECK(manager.HasBatch("portable"));

    auto outPath = axis_test_support::TempPath("portable_static_batch_out.btch");
    manager.SaveBatchToFile("portable", outPath.string());

    std::ifstream is(outPath, std::ios::binary);
    auto readU32 = [&]() {
        uint32_t value = 0;
        is.read(reinterpret_cast<char*>(&value), sizeof(value));
        return value;
    };
    auto readF32 = [&]() {
        float value = 0.0f;
        is.read(reinterpret_cast<char*>(&value), sizeof(value));
        return value;
    };

    AXIS_CHECK(readU32() == 0x48435442);
    AXIS_CHECK(readU32() == 2);
    AXIS_CHECK(readU32() == 1);
    AXIS_CHECK(readU32() == 3);

    std::vector<float> values;
    for (int i = 0; i < 8; ++i)
    {
        values.push_back(readF32());
    }

    AXIS_CHECK_NEAR(values[0], 1.0f, 0.0001f);
    AXIS_CHECK_NEAR(values[3], 4.0f, 0.0001f);
    AXIS_CHECK_NEAR(values[6], 0.25f, 0.0001f);
    AXIS_CHECK(readU32() == 0);
    AXIS_CHECK(readU32() == 0);
    AXIS_CHECK(readU32() == 0);
}

AXIS_TEST_CASE("StaticBatchManager rejects oversized and inconsistent files before allocation")
{
    auto path = axis_test_support::TempPath("unsafe_static_batch.btch");
    {
        std::ofstream os(path, std::ios::binary);
        auto writeU32 = [&](uint32_t value) { os.write(reinterpret_cast<const char*>(&value), sizeof(value)); };
        writeU32(0x48435442);
        writeU32(2);
        writeU32(10'000'001);
        writeU32(3);
    }
    StaticBatchManager manager;
    AXIS_EXPECT_ERROR_LOGS(1);
    AXIS_CHECK(!manager.LoadBatchFromFile("unsafe", path.string()));
    AXIS_CHECK(!manager.HasBatch("unsafe"));
}
