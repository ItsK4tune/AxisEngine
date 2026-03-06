#include <script/script_registry.h>

ScriptRegistry& ScriptRegistry::Instance()
{
    static ScriptRegistry instance;
    return instance;
}

std::unique_ptr<Scriptable> ScriptRegistry::Create(const std::string& name)
{
    if (m_FactoryMap.find(name) != m_FactoryMap.end())
    {
        return m_FactoryMap[name]();
    }
    LOGGER_ERROR("ScriptRegistry") << "Script not found: " << name;
    return nullptr;
}
