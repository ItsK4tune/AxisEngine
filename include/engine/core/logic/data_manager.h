#pragma once
#include <core/logic/data_node.h>
#include <string>
#include <unordered_map>

class DataManager
{
public:
    void SetDataNode(const std::string& key, const DataNode& data);
    DataNode GetDataNode(const std::string& key) const;
    bool HasDataNode(const std::string& key) const;
    void RemoveDataNode(const std::string& key);

    std::unordered_map<std::string, DataNode>& GetDataNodes()
    {
        return m_DataNodes;
    }
    const std::unordered_map<std::string, DataNode>& GetDataNodes() const
    {
        return m_DataNodes;
    }

private:
    std::unordered_map<std::string, DataNode> m_DataNodes;
};
