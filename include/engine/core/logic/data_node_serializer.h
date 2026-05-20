#pragma once
#include <core/interface/i_serializer.h>
#include <core/logic/data_node.h>
#include <core/logic/yaml_parser.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <ostream>

class DataNodeSerializer : public ISerializer<std::unordered_map<std::string, DataNode>>
{
public:
    bool Serialize(const std::string& filepath, const std::unordered_map<std::string, DataNode>& data) override;
    bool Deserialize(const std::string& filepath, std::unordered_map<std::string, DataNode>& data) override;

    static std::unordered_map<std::string, DataNode> Parse(const std::vector<YAMLNode>& roots);
    static void Serialize(std::ostream& stream, const std::unordered_map<std::string, DataNode>& data);
};
