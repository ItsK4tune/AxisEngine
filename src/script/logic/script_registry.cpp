#include <script/logic/script_registry.h>

std::unique_ptr<Scriptable> ScriptRegistry::Create(const std::string& name)
{
    auto& staticMap = GetStaticFactoryMap();
    if (staticMap.find(name) != staticMap.end())
    {
        return staticMap[name]();
    }
    
    if (m_FactoryMap.find(name) != m_FactoryMap.end())
    {
        return m_FactoryMap[name]();
    }
    LOGGER_ERROR("ScriptRegistry") << "Script not found: " << name;
    return nullptr;
}

void ScriptRegistry::Initialize()
{
    // Optional: copy from static map to instance map if we want to centralize
    auto &staticMap = GetStaticFactoryMap();
    for (auto &pair : staticMap)
    {
        if (m_FactoryMap.find(pair.first) == m_FactoryMap.end())
        {
            m_FactoryMap[pair.first] = pair.second;
        }
    }
}
