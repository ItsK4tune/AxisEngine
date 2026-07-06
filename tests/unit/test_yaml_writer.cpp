#include "test_framework.h"

#include <core/logic/yaml_parser.h>
#include <core/logic/yaml_writer.h>
#include <sstream>
#include <functional>

static bool TreesAreEqual(const std::vector<YAMLNode>& a, const std::vector<YAMLNode>& b);

static bool NodesAreEqual(const YAMLNode& a, const YAMLNode& b)
{
    if (a.key != b.key) return false;
    if (a.value != b.value) return false;
    return TreesAreEqual(a.children, b.children);
}

static bool TreesAreEqual(const std::vector<YAMLNode>& a, const std::vector<YAMLNode>& b)
{
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
    {
        if (!NodesAreEqual(a[i], b[i]))
            return false;
    }
    return true;
}

AXIS_TEST_CASE("YAMLWriter outputs valid indented text format")
{
    const std::string original =
        "axis_scene:\n"
        "  Entities:\n"
        "    Player:\n"
        "      Tag: player\n"
        "      Component: Transform\n"
        "        Position: 1 2 3\n"
        "        Rotation: 0 90 0\n"
        "      Component: Renderer\n"
        "        Model: cube\n";

    auto parsedOriginal = YAMLParser::ParseString(original);
    AXIS_CHECK(!parsedOriginal.empty());

    // Serialize using YAMLWriter
    std::string written = YAMLWriter::WriteString(parsedOriginal);

    // Re-parse the written string
    auto parsedWritten = YAMLParser::ParseString(written);
    AXIS_CHECK(!parsedWritten.empty());

    // Verify trees are identical
    AXIS_CHECK(TreesAreEqual(parsedOriginal, parsedWritten));
}

AXIS_TEST_CASE("YAMLWriter processes list syntax and key-value details properly")
{
    std::vector<YAMLNode> roots = {
        {"axis_input", "", {
            {"Bindings", "", {
                {"MoveForward", "", {
                    {"- Key", "w", {}},
                    {"- Key", "up", {}}
                }},
                {"Jump", "", {
                    {"- Key", "space", {}}
                }}
            }}
        }}
    };

    std::string written = YAMLWriter::WriteString(roots);
    
    // Parse it back
    auto parsed = YAMLParser::ParseString(written);
    AXIS_CHECK(TreesAreEqual(roots, parsed));
}

AXIS_TEST_CASE("BinaryYAMLWriter encodes and decodes YAMLNode trees seamlessly")
{
    const std::string configYaml =
        "axis_config:\n"
        "  TITLE: AxisEngine Game\n"
        "  LOG_LEVEL: DEBUG\n"
        "  WINDOW_WIDTH: 1920\n"
        "  WINDOW_HEIGHT: 1080\n"
        "  CLEAR_COLOR: 0.1 0.2 0.3 1.0\n";

    auto originalTree = YAMLParser::ParseString(configYaml);
    AXIS_CHECK(!originalTree.empty());

    // Encode to stream
    std::stringstream binaryStream(std::ios::in | std::ios::out | std::ios::binary);
    BinaryYAMLWriter::Write(binaryStream, originalTree);

    // Decode from stream
    binaryStream.seekg(0);
    auto decodedTree = BinaryYAMLWriter::Read(binaryStream);

    AXIS_CHECK(!decodedTree.empty());
    AXIS_CHECK(TreesAreEqual(originalTree, decodedTree));
}
