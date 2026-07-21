#pragma once
#include <core/logic/data_node.h>
#include <shared_mutex>
#include <string>
#include <unordered_map>

class DataManager
{
public:
    void SetDataNode(const std::string& key, const DataNode& data);
    DataNode GetDataNode(const std::string& key) const;
    bool HasDataNode(const std::string& key) const;
    void RemoveDataNode(const std::string& key);

    std::unordered_map<std::string, DataNode> GetDataNodes() const;
    void ReplaceDataNodes(std::unordered_map<std::string, DataNode> data);

private:
    std::unordered_map<std::string, DataNode> m_DataNodes;
    mutable std::shared_mutex m_Mutex;
};
