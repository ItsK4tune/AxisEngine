#include <script/logic/script_registry.h>
#include <core/logic/service_locator.h>

std::unique_ptr<IScriptable> ScriptRegistry::Create(const std::string& name)
{
    if (name.empty())
        return nullptr;

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
    ServiceLocator::Instance().Register<IScriptRegistry>(this);

    auto& staticMap = GetStaticFactoryMap();
    for (auto& pair : staticMap)
    {
        if (m_FactoryMap.find(pair.first) == m_FactoryMap.end())
        {
            m_FactoryMap[pair.first] = pair.second;
        }
    }
}
