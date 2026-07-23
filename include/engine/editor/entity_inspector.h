#pragma once

#include <entt/entt.hpp>
#include <array>
#include <string>

class EntityInspector
{
public:
    void Draw(entt::registry& registry, entt::entity entity);

private:
    entt::entity m_InfoEditEntity = entt::null;
    std::array<char, 256> m_InfoNameBuffer{};
    std::array<char, 256> m_InfoTagBuffer{};
    bool m_NameEditUndoCaptured = false;
    bool m_TagEditUndoCaptured = false;
    std::string m_NewPostProcessShader;
    int m_SelectedAnimationPreview = 0;
};
