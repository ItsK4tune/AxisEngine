#include "test_framework.h"

#include <core/logic/yaml_parser.h>

AXIS_TEST_CASE("YAMLParser parses nested axis scene component nodes")
{
    const std::string content =
        "axis_scene:\n"
        "  Entities:\n"
        "    Player:\n"
        "      Tag: player\n"
        "      Component: Transform\n"
        "        Position: 1 2 3\n"
        "        Rotation: 0 90 0\n";

    auto roots = YAMLParser::ParseString(content);

    AXIS_CHECK(roots.size() == 1);
    AXIS_CHECK(roots[0].key == "axis_scene");

    auto* entities = roots[0].GetChild("Entities");
    AXIS_CHECK(entities != nullptr);
    AXIS_CHECK(entities->children.size() == 1);
    AXIS_CHECK(entities->children[0].key == "Player");

    auto* tag = entities->children[0].GetChild("Tag");
    AXIS_CHECK(tag != nullptr);
    AXIS_CHECK(tag->value == "player");

    auto* component = entities->children[0].GetChild("Component");
    AXIS_CHECK(component != nullptr);
    AXIS_CHECK(component->value == "Transform");
    AXIS_CHECK(component->GetChildValue("Position") == "1 2 3");
    AXIS_CHECK(component->GetChildValue("Rotation") == "0 90 0");
}

AXIS_TEST_CASE("YAMLParser ignores blank lines and full-line comments")
{
    const std::string content =
        "# Root comment\n"
        "\n"
        "axis_scene:\n"
        "  # Entity block comment\n"
        "  Entities:\n"
        "    Player:\n"
        "      Component: Camera\n";

    auto roots = YAMLParser::ParseString(content);

    AXIS_CHECK(roots.size() == 1);
    auto* entities = roots[0].GetChild("Entities");
    AXIS_CHECK(entities != nullptr);
    AXIS_CHECK(entities->children.size() == 1);
    AXIS_CHECK(entities->children[0].key == "Player");
    AXIS_CHECK(entities->children[0].GetChildValue("Component") == "Camera");
}

AXIS_TEST_CASE("YAMLParser rejects tab indentation without partially parsed output")
{
    AXIS_EXPECT_ERROR_LOGS(1);
    const std::string content =
        "axis_scene:\n"
        "  Entities:\n"
        "\tPlayer:\n"
        "    Component: Camera\n";

    const auto roots = YAMLParser::ParseString(content);

    AXIS_CHECK(roots.empty());
}

AXIS_TEST_CASE("YAMLNode merge overrides matching component and appends new component")
{
    auto base = YAMLParser::ParseString(
        "Entity:\n"
        "  Component: Renderer\n"
        "    Model: cube\n"
        "    Color: 1 1 1 1\n");

    auto overrideNodes = YAMLParser::ParseString(
        "Entity:\n"
        "  Component: Renderer\n"
        "    Color: 1 0 0 1\n"
        "  Component: Material\n"
        "    Roughness: 0.2\n");

    YAMLNode::Merge(base[0], overrideNodes[0]);

    AXIS_CHECK(base[0].children.size() == 2);
    AXIS_CHECK(base[0].children[0].key == "Component");
    AXIS_CHECK(base[0].children[0].value == "Renderer");
    AXIS_CHECK(base[0].children[0].GetChildValue("Model") == "cube");
    AXIS_CHECK(base[0].children[0].GetChildValue("Color") == "1 0 0 1");
    AXIS_CHECK(base[0].children[1].value == "Material");
    AXIS_CHECK(base[0].children[1].GetChildValue("Roughness") == "0.2");
}

AXIS_TEST_CASE("YAMLParser parses list item syntax")
{
    const std::string content =
        "Tags:\n"
        "  - walkable\n"
        "  - road\n";

    auto roots = YAMLParser::ParseString(content);

    AXIS_CHECK(roots.size() == 1);
    AXIS_CHECK(roots[0].key == "Tags");
    AXIS_CHECK(roots[0].children.size() == 2);
    AXIS_CHECK(roots[0].children[0].key == "-");
    AXIS_CHECK(roots[0].children[0].value == "walkable");
    AXIS_CHECK(roots[0].children[1].key == "-");
    AXIS_CHECK(roots[0].children[1].value == "road");
}
