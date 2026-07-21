#include <core/logic/data_manager.h>
#include <mutex>
#include <utility>

void DataManager::SetDataNode(const std::string& key, const DataNode& data)
{
    std::unique_lock lock(m_Mutex);
    m_DataNodes[key] = data;
}

DataNode DataManager::GetDataNode(const std::string& key) const
{
    std::shared_lock lock(m_Mutex);
    auto it = m_DataNodes.find(key);
    if (it != m_DataNodes.end())
        return it->second;
    return {};
}

bool DataManager::HasDataNode(const std::string& key) const
{
    std::shared_lock lock(m_Mutex);
    return m_DataNodes.find(key) != m_DataNodes.end();
}

void DataManager::RemoveDataNode(const std::string& key)
{
    std::unique_lock lock(m_Mutex);
    m_DataNodes.erase(key);
}

std::unordered_map<std::string, DataNode> DataManager::GetDataNodes() const
{
    std::shared_lock lock(m_Mutex);
    return m_DataNodes;
}

void DataManager::ReplaceDataNodes(std::unordered_map<std::string, DataNode> data)
{
    std::unique_lock lock(m_Mutex);
    m_DataNodes = std::move(data);
}
