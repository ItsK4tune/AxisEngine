#pragma once

#include <core/logic/yaml_parser.h>
#include <entt/entt.hpp>

class IComponentSerializerFactory
{
public:
    virtual ~IComponentSerializerFactory() = default;

    // Return true and populate `component.children` when the entity owns the
    // module component. Runtime-only fields should not be emitted.
    virtual bool Serialize(const entt::registry& registry, entt::entity entity, YAMLNode& component) const = 0;
};
