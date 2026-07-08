#pragma once

#include <core/interface/i_base_system.h>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

class SystemFactory
{
public:
    using Creator = std::function<std::unique_ptr<IBaseSystem>()>;

    static void Register(const std::string& name, Creator creator);
    static std::unique_ptr<IBaseSystem> Create(const std::string& name);
    static std::vector<std::unique_ptr<IBaseSystem>> CreateAll();

private:
    static std::map<std::string, Creator>& GetRegistry();
};

#define REGISTER_SYSTEM(SystemType)                                                            \
    static bool SystemType##_Registered = []() {                                               \
        SystemFactory::Register(#SystemType, []() { return std::make_unique<SystemType>(); }); \
        return true;                                                                           \
    }();
