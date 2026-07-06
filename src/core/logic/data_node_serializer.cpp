#include <core/logic/data_node_serializer.h>
#include <core/logic/yaml_parser.h>
#include <core/logic/yaml_writer.h>
#include <core/logic/filesystem.h>
#include <core/logic/logger.h>
#include <fstream>

std::unordered_map<std::string, DataNode> DataNodeSerializer::Parse(const std::vector<YAMLNode>& roots)
{
    std::unordered_map<std::string, DataNode> result;

    // Check top-level roots first
    for (auto& r : roots)
    {
        if (r.key == "axis_data")
        {
            for (auto& node : r.children)
            {
                DataNode data;
                data.value = node.value;
                for (auto& attrNode : node.children)
                {
                    data.attributes[attrNode.key] = attrNode.value;
                }
                result[node.key] = data;
            }
        }
    }

    // Check inside axis_scene if present (case where it's nested)
    for (auto& r : roots)
    {
        if (r.key == "axis_scene")
        {
            for (auto& child : r.children)
            {
                if (child.key == "axis_data")
                {
                    for (auto& node : child.children)
                    {
                        DataNode data;
                        data.value = node.value;
                        for (auto& attrNode : node.children)
                        {
                            data.attributes[attrNode.key] = attrNode.value;
                        }
                        result[node.key] = data;
                    }
                }
            }
        }
    }

    return result;
}

void DataNodeSerializer::Serialize(std::ostream& stream, const std::unordered_map<std::string, DataNode>& data)
{
    if (data.empty())
        return;

    // Build a YAMLNode tree and delegate to YAMLWriter
    std::vector<YAMLNode> nodes;
    nodes.reserve(data.size());
    for (const auto& [key, val] : data)
    {
        YAMLNode node{key, val.value, {}};
        for (const auto& [attrKey, attrVal] : val.attributes)
            node.children.push_back({attrKey, attrVal, {}});
        nodes.push_back(std::move(node));
    }

    YAMLWriter::WriteSection(stream, "axis_data", nodes);
}

bool DataNodeSerializer::Deserialize(const std::string& filepath, std::unordered_map<std::string, DataNode>& data)
{
    std::string fullPath = FileSystem::getPath(filepath);
    auto roots = YAMLParser::Parse(fullPath);

    data = Parse(roots);

    for (auto& r : roots)
    {
        if (r.key.rfind("axis_", 0) == 0 && r.key != "axis_data" && r.key != "axis_scene")
        {
            LOGGER_WARN("DataNodeSerializer") << "Potential typo in root key: '" << r.key << "', expected 'axis_data' in " << filepath;
        }
    }

    // Fallback: If no "axis_data" root was found but other non-scene roots exist, treat them as data
    bool hasAxisDataRoot = false;
    for (auto& r : roots)
    {
        if (r.key == "axis_data" || (r.key == "axis_scene" && r.GetChild("axis_data")))
        {
            hasAxisDataRoot = true;
            break;
        }
    }

    if (!hasAxisDataRoot)
    {
        for (auto& r : roots)
        {
            if (r.key != "axis_scene")
            {
                DataNode d;
                d.value = r.value;
                for (auto& attrNode : r.children)
                {
                    d.attributes[attrNode.key] = attrNode.value;
                }
                data[r.key] = d;
            }
        }
    }

    return !data.empty();
}

bool DataNodeSerializer::Serialize(const std::string& filepath, const std::unordered_map<std::string, DataNode>& data)
{
    std::string fullPath = FileSystem::getPath(filepath);
    std::ofstream f(fullPath);
    if (!f.is_open())
        return false;

    Serialize(f, data);
    return true;
}
