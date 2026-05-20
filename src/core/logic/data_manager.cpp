#include <core/logic/data_manager.h>

void DataManager::SetDataNode(const std::string& key, const DataNode& data)
{
    m_DataNodes[key] = data;
}

DataNode DataManager::GetDataNode(const std::string& key) const
{
    auto it = m_DataNodes.find(key);
    if (it != m_DataNodes.end())
        return it->second;
    return {};
}

bool DataManager::HasDataNode(const std::string& key) const
{
    return m_DataNodes.find(key) != m_DataNodes.end();
}

void DataManager::RemoveDataNode(const std::string& key)
{
    m_DataNodes.erase(key);
}
