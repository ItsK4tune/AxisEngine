#pragma once

#include <core/interface/i_base_system.h>
#include <memory>
#include <string>
#include <typeindex>

class ISystemRegistry
{
public:
    virtual ~ISystemRegistry() = default;

    virtual void RegisterSystem(std::unique_ptr<IBaseSystem> system) = 0;
    virtual IBaseSystem* GetSystem(const std::string& name) const = 0;
    virtual IBaseSystem* GetSystem(SystemId id) const = 0;
    virtual IBaseSystem* GetSystem(std::type_index concreteType) const = 0;

    template <typename T>
    T* GetSystem() const
    {
        return dynamic_cast<T*>(GetSystem(std::type_index(typeid(T))));
    }
};
