#pragma once

#include <core/interface/i_base_system.h>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class SystemFactory
{
public:
    using Creator = std::function<std::unique_ptr<IBaseSystem>()>;

    static std::unique_ptr<IBaseSystem> Create(const std::string& name);
    static std::vector<std::unique_ptr<IBaseSystem>> CreateAll();
    static std::vector<std::string> GetRegisteredNames();

private:
    static void RegisterDefault(const std::string& name, Creator creator);
    static void EnsureBuiltInSystemsRegistered();
    static std::map<std::string, Creator>& GetRegistry();
    static std::mutex& GetMutex();
};
