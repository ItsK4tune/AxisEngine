#include <ecs/logic/system_factory.h>

std::map<std::string, SystemFactory::Creator>& SystemFactory::GetRegistry()
{
    static std::map<std::string, Creator> registry;
    return registry;
}

void SystemFactory::Register(const std::string& name, Creator creator)
{
    GetRegistry()[name] = creator;
}

std::unique_ptr<IBaseSystem> SystemFactory::Create(const std::string& name)
{
    auto it = GetRegistry().find(name);
    if (it != GetRegistry().end())
        return it->second();
    return nullptr;
}

std::vector<std::unique_ptr<IBaseSystem>> SystemFactory::CreateAll()
{
    std::vector<std::unique_ptr<IBaseSystem>> systems;
    for (auto& pair : GetRegistry())
    {
        if (auto sys = pair.second())
        {
            systems.push_back(std::move(sys));
        }
    }
    return systems;
}
