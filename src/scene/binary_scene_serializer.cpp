#include <scene/binary_scene_serializer.h>
#include <ecs/components/info_component.h>
#include <ecs/components/transform_component.h>
#include <ecs/entity_manager.h>
#include <rendering/geometry/model.h>
#include <rendering/core/shader.h>
#include <core/utils/logger.h>
#include <fstream>
#include <vector>

bool BinarySceneSerializer::Serialize(const std::string& filepath, Scene& scene)
{
    std::ofstream os(filepath, std::ios::binary);
    if (!os.is_open()) return false;

    os.write(reinterpret_cast<const char*>(&MAGIC), sizeof(MAGIC));
    os.write(reinterpret_cast<const char*>(&VERSION), sizeof(VERSION));

    auto view = scene.registry.view<InfoComponent>();
    std::vector<entt::entity> entities;
    for (auto entity : view) entities.push_back(entity);
    uint32_t entityCount = (uint32_t)entities.size();
    os.write(reinterpret_cast<const char*>(&entityCount), sizeof(entityCount));

    std::map<entt::entity, uint32_t> entityToIndex;
    uint32_t idx = 0;
    for (auto entity : entities) entityToIndex[entity] = idx++;

    for (auto entity : entities)
    {
        auto& info = view.get<InfoComponent>(entity);
        
        auto writeString = [&](const std::string& s) {
            uint32_t len = (uint32_t)s.length();
            os.write(reinterpret_cast<const char*>(&len), sizeof(len));
            os.write(s.c_str(), len);
        };

        writeString(info.name);
        writeString(info.tag);
        os.write(reinterpret_cast<const char*>(&info.layer), sizeof(info.layer));
        
        auto* pos = scene.registry.try_get<PositionComponent>(entity);
        auto* rot = scene.registry.try_get<RotationComponent>(entity);
        auto* scl = scene.registry.try_get<ScaleComponent>(entity);
        
        bool hasP = (pos != nullptr); os.write(reinterpret_cast<const char*>(&hasP), sizeof(hasP));
        if (hasP) os.write(reinterpret_cast<const char*>(&pos->value), sizeof(pos->value));

        bool hasR = (rot != nullptr); os.write(reinterpret_cast<const char*>(&hasR), sizeof(hasR));
        if (hasR) os.write(reinterpret_cast<const char*>(&rot->value), sizeof(rot->value));

        bool hasS = (scl != nullptr); os.write(reinterpret_cast<const char*>(&hasS), sizeof(hasS));
        if (hasS) os.write(reinterpret_cast<const char*>(&scl->value), sizeof(scl->value));

        auto* hier = scene.registry.try_get<HierarchyComponent>(entity);
        int32_t parentIdx = -1;
        if (hier && hier->parent != entt::null && entityToIndex.count(hier->parent))
            parentIdx = (int32_t)entityToIndex[hier->parent];
        os.write(reinterpret_cast<const char*>(&parentIdx), sizeof(parentIdx));

        auto* mesh = scene.registry.try_get<MeshRendererComponent>(entity);
        bool hasMesh = (mesh != nullptr);
        os.write(reinterpret_cast<const char*>(&hasMesh), sizeof(hasMesh));
        if (hasMesh)
        {
            writeString(mesh->model ? mesh->model->GetName() : "");
            auto s = mesh->shader.lock();
            writeString(s ? s->GetName() : "");
            os.write(reinterpret_cast<const char*>(&mesh->castShadow), sizeof(mesh->castShadow));
        }

        auto* mat = scene.registry.try_get<MaterialComponent>(entity);
        bool hasMat = (mat != nullptr);
        os.write(reinterpret_cast<const char*>(&hasMat), sizeof(hasMat));
        if (hasMat)
        {
            os.write(reinterpret_cast<const char*>(&mat->desc), sizeof(mat->desc));
        }
    }

    os.close();
    LOGGER_INFO("BinarySceneSerializer") << "Serialized scene to: " << filepath;
    return true;
}

bool BinarySceneSerializer::Deserialize(const std::string& filepath, Scene& scene, ResourceManager& res, IPhysicsWorld& phys, SoundPlayer& sound, EngineContext ctx)
{
    std::ifstream is(filepath, std::ios::binary);
    if (!is.is_open()) return false;

    uint32_t magic, version;
    is.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    is.read(reinterpret_cast<char*>(&version), sizeof(version));

    if (magic != MAGIC || version != VERSION) return false;

    uint32_t entityCount;
    is.read(reinterpret_cast<char*>(&entityCount), sizeof(entityCount));

    std::vector<entt::entity> entities;
    std::vector<int32_t> parents;

    auto readString = [&]() {
        uint32_t len;
        is.read(reinterpret_cast<char*>(&len), sizeof(len));
        std::string s(len, ' ');
        if (len > 0) is.read(&s[0], len);
        return s;
    };

    for (uint32_t i = 0; i < entityCount; ++i)
    {
        std::string name = readString();
        std::string tag = readString();
        uint32_t layer;
        is.read(reinterpret_cast<char*>(&layer), sizeof(layer));

        entt::entity entity = scene.registry.create();
        entities.push_back(entity);

        auto& info = scene.registry.emplace<InfoComponent>(entity, name, tag);
        info.layer = layer;

        bool hasP, hasR, hasS;
        is.read(reinterpret_cast<char*>(&hasP), sizeof(hasP));
        if (hasP) { glm::vec3 v; is.read(reinterpret_cast<char*>(&v), sizeof(v)); scene.registry.emplace<PositionComponent>(entity, v, v); }

        is.read(reinterpret_cast<char*>(&hasR), sizeof(hasR));
        if (hasR) { glm::quat v; is.read(reinterpret_cast<char*>(&v), sizeof(v)); scene.registry.emplace<RotationComponent>(entity, v, v); }

        is.read(reinterpret_cast<char*>(&hasS), sizeof(hasS));
        if (hasS) { glm::vec3 v; is.read(reinterpret_cast<char*>(&v), sizeof(v)); scene.registry.emplace<ScaleComponent>(entity, v, v); }
        
        int32_t pIdx; is.read(reinterpret_cast<char*>(&pIdx), sizeof(pIdx));
        parents.push_back(pIdx);

        bool hasMesh; is.read(reinterpret_cast<char*>(&hasMesh), sizeof(hasMesh));
        if (hasMesh)
        {
            std::string modelName = readString();
            std::string shaderName = readString();
            bool castShadow; is.read(reinterpret_cast<char*>(&castShadow), sizeof(castShadow));
            
            auto& m = scene.registry.emplace<MeshRendererComponent>(entity);
            m.model = res.GetModel(modelName);
            m.shader = res.GetShader(shaderName);
            m.castShadow = castShadow;
        }

        bool hasMat; is.read(reinterpret_cast<char*>(&hasMat), sizeof(hasMat));
        if (hasMat)
        {
            auto& m = scene.registry.emplace<MaterialComponent>(entity);
            is.read(reinterpret_cast<char*>(&m.desc), sizeof(m.desc));
            m.gpu.dirty = true;
        }

        scene.registry.emplace<HierarchyComponent>(entity);
        scene.registry.emplace<WorldTransformComponent>(entity);
    }

    for (size_t i = 0; i < entities.size(); ++i)
    {
        if (parents[i] != -1 && (size_t)parents[i] < entities.size())
        {
            EntityManager::SetParent(scene, entities[i], entities[parents[i]]);
        }
    }

    is.close();
    LOGGER_INFO("BinarySceneSerializer") << "Deserialized scene: " << filepath;
    return true;
}
