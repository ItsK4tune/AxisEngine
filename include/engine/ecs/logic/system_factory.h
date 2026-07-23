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

    // Registers an optional system module before CreateAll() is called.
    // Linking Axis::Editor uses this entry point to add EditorSystem without
    // making the runtime-only axis_engine target depend on editor code.
    static bool Register(std::string name, Creator creator);
    static std::unique_ptr<IBaseSystem> Create(const std::string& name);
    static std::vector<std::unique_ptr<IBaseSystem>> CreateAll();
    static std::vector<std::string> GetRegisteredNames();

private:
    static void EnsureBuiltInSystemsRegistered();
    static std::map<std::string, Creator>& GetRegistry();
    static std::mutex& GetMutex();
};
