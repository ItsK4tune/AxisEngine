#include <script/logic/script_registry.h>
#include <script/logic/default_camera_controller.h>
#include <core/logic/service_locator.h>
#include <algorithm>

bool ScriptRegistry::RegisterFactory(const std::string& name, ScriptFactory factory)
{
    if (name.empty() || !factory)
        return false;

    {
        std::lock_guard lock(m_FactoryMutex);
        m_FactoryMap[name] = std::move(factory);
    }
    LOGGER_INFO("ScriptRegistry") << "Registered script: " << name;
    return true;
}

std::unique_ptr<IScriptable> ScriptRegistry::Create(const std::string& name)
{
    if (name.empty())
        return nullptr;

    ScriptFactory factory;
    {
        std::lock_guard lock(m_FactoryMutex);
        if (const auto it = m_FactoryMap.find(name); it != m_FactoryMap.end())
            factory = it->second;
    }
    if (factory)
        return factory();
    LOGGER_ERROR("ScriptRegistry") << "Script not found: " << name;
    return nullptr;
}

std::vector<std::string> ScriptRegistry::GetRegisteredNames() const
{
    std::vector<std::string> names;
    {
        std::lock_guard lock(m_FactoryMutex);
        names.reserve(m_FactoryMap.size());
        for (const auto& [name, factory] : m_FactoryMap)
        {
            if (factory)
                names.push_back(name);
        }
    }
    std::sort(names.begin(), names.end());
    return names;
}

void ScriptRegistry::Initialize()
{
    ServiceLocator::Instance().Register<IScriptRegistry>(this);
    RegisterFactory("DefaultCameraController", []() -> std::unique_ptr<IScriptable> {
        return std::make_unique<DefaultCameraController>();
    });
}
