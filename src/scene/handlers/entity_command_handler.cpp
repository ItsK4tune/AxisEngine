#include <scene/handlers/entity_command_handler.h>
#include <scene/scene.h>
#include <ecs/component.h>
#include <utils/logger.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace SceneHandlers
{
    entt::entity EntityCommandHandler::HandleNewEntity(std::stringstream& ss, Scene& scene)
    {
        entt::entity entity = scene.createEntity();
        
        std::string entityName = "unnamed";
        std::string entityTag = "default";
        
        if (ss >> entityName)
        {
            ss >> entityTag;
        }
        
        scene.registry.emplace<InfoComponent>(entity, entityName, entityTag);
        return entity;
    }

    void EntityCommandHandler::HandleTransform(std::stringstream& ss, Scene& scene, entt::entity entity)
    {
        float x, y, z, rx, ry, rz, sx, sy, sz;
        ss >> x >> y >> z >> rx >> ry >> rz >> sx >> sy >> sz;
        
        auto& t = scene.registry.get_or_emplace<TransformComponent>(entity);
        t.position = glm::vec3(x, y, z);
        t.rotation = glm::quat(glm::radians(glm::vec3(rx, ry, rz)));
        t.scale = glm::vec3(sx, sy, sz);
    }

    void EntityCommandHandler::HandleParent(std::stringstream& ss, Scene& scene, entt::entity entity)
    {
        std::string parentName;
        ss >> parentName;

        entt::entity parentEntity = entt::null;
        auto view = scene.registry.view<InfoComponent>();
        
        for (auto e : view)
        {
            const auto& info = view.get<InfoComponent>(e);
            if (info.name == parentName)
            {
                parentEntity = e;
                break;
            }
        }

        if (parentEntity != entt::null)
        {
            if (scene.registry.all_of<TransformComponent>(entity) && 
                scene.registry.all_of<TransformComponent>(parentEntity))
            {
                auto& transform = scene.registry.get<TransformComponent>(entity);
                transform.SetParent(entity, parentEntity, scene.registry, true);
            }
        }
        else
        {
            LOGGER_ERROR("EntityCommandHandler") << "Parent not found: " << parentName;
        }
    }

    void EntityCommandHandler::HandleChildren(
        std::stringstream& ss, 
        entt::entity entity,
        std::map<entt::entity, std::vector<std::string>>& deferredChildren)
    {
        std::string childName;
        while (ss >> childName)
        {
            deferredChildren[entity].push_back(childName);
        }
    }
}
