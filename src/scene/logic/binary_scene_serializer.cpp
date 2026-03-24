#include <scene/logic/binary_scene_serializer.h>
#include <scene/type/scene_types.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/light_components.h>
#include <ecs/unit/render_components.h>
#include <ecs/logic/entity_manager.h>
#include <resource/unit/model.h>
#include <resource/unit/shader.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <resource/logic/resource_manager.h>
#include <physics/interface/i_physics_world.h>
#include <engine/platform/logic/io_handler.h>
#include <fstream>
#include <vector>
#include <map>
#include <ecs/unit/media_components.h>
#include <core/logic/config_manager.h>
#include <core/type/app_config.h>

bool BinarySceneSerializer::Save(const std::string& path, Scene& scene)
{
    auto& sl = ServiceLocator::Instance();
    auto& rm = sl.Require<ResourceManager>();
    auto& io = sl.Require<IOHandler>();
    std::ofstream os(path, std::ios::binary);
    if (!os.is_open()) return false;
    os.write(reinterpret_cast<const char*>(&scene::BINARY_MAGIC), sizeof(scene::BINARY_MAGIC));
    os.write(reinterpret_cast<const char*>(&scene::BINARY_VERSION), sizeof(scene::BINARY_VERSION));
    auto view = scene.registry.view<InfoComponent>();
    std::vector<entt::entity> entities;
    for (auto entity : view) entities.push_back(entity);
    uint32_t entityCount = (uint32_t)entities.size();
    os.write(reinterpret_cast<const char*>(&entityCount), sizeof(entityCount));


    auto& configMgr = sl.Require<ConfigManager>();
    const AppConfig& config = configMgr.GetConfig();
    
    auto writeString = [&](const std::string& s) {
        uint32_t len = (uint32_t)s.length();
        os.write(reinterpret_cast<const char*>(&len), sizeof(len));
        os.write(s.c_str(), len);
    };

    writeString(config.title);
    os.write(reinterpret_cast<const char*>(&config.logLevel), sizeof(config.logLevel));
    os.write(reinterpret_cast<const char*>(&config.numJobThreads), sizeof(config.numJobThreads));
    os.write(reinterpret_cast<const char*>(&config.timeScale), sizeof(config.timeScale));
    writeString(config.iconPath);
    writeString(config.audioDevice);
    os.write(reinterpret_cast<const char*>(&config.width), 4 * 7);
    os.write(reinterpret_cast<const char*>(&config.graphicsBackend), sizeof(config.graphicsBackend));
    os.write(reinterpret_cast<const char*>(&config.msaaSamples), 4 * 3);
    os.write(reinterpret_cast<const char*>(&config.renderScale), 4 * 1);
    os.write(reinterpret_cast<const char*>(&config.asyncResourceLoading), sizeof(bool));
    os.write(reinterpret_cast<const char*>(&config.renderPath), sizeof(config.renderPath));
    os.write(reinterpret_cast<const char*>(&config.tonemappingMode), sizeof(config.tonemappingMode));
    os.write(reinterpret_cast<const char*>(&config.hdrEnabled), sizeof(bool) * 2);
    os.write(reinterpret_cast<const char*>(&config.gamma), 4 * 6);
    os.write(reinterpret_cast<const char*>(&config.clearColor), 4 * 4);
    os.write(reinterpret_cast<const char*>(&config.shadowsEnabled), sizeof(bool));
    os.write(reinterpret_cast<const char*>(&config.shadowMode), 4 * 3);
    os.write(reinterpret_cast<const char*>(&config.shadowFrustumCullingEnabled), sizeof(bool));
    os.write(reinterpret_cast<const char*>(&config.shadowDistanceCulling), 4 * 3);
    os.write(reinterpret_cast<const char*>(&config.physicsBackend), sizeof(config.physicsBackend));
    os.write(reinterpret_cast<const char*>(&config.physicsMode), sizeof(config.physicsMode));
    os.write(reinterpret_cast<const char*>(&config.gravity), 4 * 3);
    os.write(reinterpret_cast<const char*>(&config.maxSubSteps), 4 * 2);
    os.write(reinterpret_cast<const char*>(&config.ccdEnabled), sizeof(bool));
    os.write(reinterpret_cast<const char*>(&config.ccdThreshold), 4 * 2);
    os.write(reinterpret_cast<const char*>(&config.mouseSensitivityX), 4 * 2);
    os.write(reinterpret_cast<const char*>(&config.mouseInvertX), sizeof(bool) * 2);
    os.write(reinterpret_cast<const char*>(&config.audioBackend), sizeof(config.audioBackend));
    os.write(reinterpret_cast<const char*>(&config.masterVolume), 4);
    os.write(reinterpret_cast<const char*>(&config.cullFaceEnabled), sizeof(bool) * 5);
    os.write(reinterpret_cast<const char*>(&config.renderOrderEnabled), sizeof(bool));
    os.write(reinterpret_cast<const char*>(&config.filterLayerMask), 4 * 2);

    std::map<entt::entity, uint32_t> entityToIndex;
    uint32_t idx = 0;
    for (auto entity : entities) entityToIndex[entity] = idx++;

    for (auto entity : entities)
    {
        auto& info = scene.registry.get<InfoComponent>(entity);
        
        {
            uint32_t len = (uint32_t)info.name.length();
            os.write(reinterpret_cast<const char*>(&len), sizeof(len));
            os.write(info.name.c_str(), len);
        }
        {
            uint32_t len = (uint32_t)info.tag.length();
            os.write(reinterpret_cast<const char*>(&len), sizeof(len));
            os.write(info.tag.c_str(), len);
        }
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
            std::string mName = mesh->model ? mesh->model->GetName() : "";
            uint32_t len = (uint32_t)mName.length();
            os.write(reinterpret_cast<const char*>(&len), sizeof(len));
            os.write(mName.c_str(), len);

            auto s_ptr = mesh->shader.lock();
            std::string sName = s_ptr ? s_ptr->GetName() : "";
            len = (uint32_t)sName.length();
            os.write(reinterpret_cast<const char*>(&len), sizeof(len));
            os.write(sName.c_str(), len);

            os.write(reinterpret_cast<const char*>(&mesh->castShadow), sizeof(mesh->castShadow));
        }

        auto* mat = scene.registry.try_get<MaterialComponent>(entity);
        bool hasMat = (mat != nullptr);
        os.write(reinterpret_cast<const char*>(&hasMat), sizeof(hasMat));
        if (hasMat)
        {
            os.write(reinterpret_cast<const char*>(&mat->desc), sizeof(mat->desc));
        }


        auto* cam = scene.registry.try_get<CameraComponent>(entity);
        bool hasCam = (cam != nullptr);
        os.write(reinterpret_cast<const char*>(&hasCam), sizeof(hasCam));
        if (hasCam)
        {
            os.write(reinterpret_cast<const char*>(cam), sizeof(CameraComponent));
        }


        auto* dl = scene.registry.try_get<DirectionalLightComponent>(entity);
        bool hasDL = (dl != nullptr);
        os.write(reinterpret_cast<const char*>(&hasDL), sizeof(hasDL));
        if (hasDL) os.write(reinterpret_cast<const char*>(dl), sizeof(DirectionalLightComponent));

        auto* pl = scene.registry.try_get<PointLightComponent>(entity);
        bool hasPL = (pl != nullptr);
        os.write(reinterpret_cast<const char*>(&hasPL), sizeof(hasPL));
        if (hasPL) os.write(reinterpret_cast<const char*>(pl), sizeof(PointLightComponent));

        auto* sl_comp = scene.registry.try_get<SpotLightComponent>(entity);
        bool hasSL = (sl_comp != nullptr);
        os.write(reinterpret_cast<const char*>(&hasSL), sizeof(hasSL));
        if (hasSL) os.write(reinterpret_cast<const char*>(sl_comp), sizeof(SpotLightComponent));


        auto* sky = scene.registry.try_get<SkyboxRenderComponent>(entity);
        bool hasSky = (sky != nullptr);
        os.write(reinterpret_cast<const char*>(&hasSky), sizeof(hasSky));
        if (hasSky)
        {
            os.write(reinterpret_cast<const char*>(&sky->isPrimary), sizeof(sky->isPrimary));
            std::string sName = sky->skybox ? sky->skybox->GetName() : "";
            uint32_t len = (uint32_t)sName.length();
            os.write(reinterpret_cast<const char*>(&len), sizeof(len));
            os.write(sName.c_str(), len);

            auto sh_ptr = sky->shader.lock();
            std::string shName = sh_ptr ? sh_ptr->GetName() : "";
            len = (uint32_t)shName.length();
            os.write(reinterpret_cast<const char*>(&len), sizeof(len));
            os.write(shName.c_str(), len);
        }
    }

    os.close();
    LOGGER_INFO("BinarySceneSerializer") << "Serialized scene to: " << path;
    return true;
}

bool BinarySceneSerializer::Load(const std::string& path, Scene& scene)
{
    auto& sl = ServiceLocator::Instance();
    auto& rm = sl.Require<ResourceManager>();
    auto& physics = sl.Require<IPhysicsWorld>();

    std::ifstream is(path, std::ios::binary);
    if (!is.is_open()) return false;

    uint32_t magic, version;
    is.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    is.read(reinterpret_cast<char*>(&version), sizeof(version));
    
    if (magic != scene::BINARY_MAGIC) return false;

    uint32_t entityCount;
    is.read(reinterpret_cast<char*>(&entityCount), sizeof(entityCount));

    auto readString = [&]() -> std::string {
        uint32_t len;
        is.read(reinterpret_cast<char*>(&len), sizeof(len));
        std::string s(len, ' ');
        is.read(&s[0], len);
        return s;
    };

    if (version >= 2)
    {
        auto& configMgr = sl.Require<ConfigManager>();
        AppConfig config = configMgr.GetConfig();

        config.title = readString();
        is.read(reinterpret_cast<char*>(&config.logLevel), sizeof(config.logLevel));
        is.read(reinterpret_cast<char*>(&config.numJobThreads), sizeof(config.numJobThreads));
        is.read(reinterpret_cast<char*>(&config.timeScale), sizeof(config.timeScale));
        config.iconPath = readString();
        config.audioDevice = readString();
        is.read(reinterpret_cast<char*>(&config.width), 4 * 7);
        is.read(reinterpret_cast<char*>(&config.graphicsBackend), sizeof(config.graphicsBackend));
        is.read(reinterpret_cast<char*>(&config.msaaSamples), 4 * 3);
        is.read(reinterpret_cast<char*>(&config.renderScale), 4 * 1);
        is.read(reinterpret_cast<char*>(&config.asyncResourceLoading), sizeof(bool));
        is.read(reinterpret_cast<char*>(&config.renderPath), sizeof(config.renderPath));
        is.read(reinterpret_cast<char*>(&config.tonemappingMode), sizeof(config.tonemappingMode));
        is.read(reinterpret_cast<char*>(&config.hdrEnabled), sizeof(bool) * 2);
        is.read(reinterpret_cast<char*>(&config.gamma), 4 * 6);
        is.read(reinterpret_cast<char*>(&config.clearColor), 4 * 4);
        is.read(reinterpret_cast<char*>(&config.shadowsEnabled), sizeof(bool));
        is.read(reinterpret_cast<char*>(&config.shadowMode), 4 * 3);
        is.read(reinterpret_cast<char*>(&config.shadowFrustumCullingEnabled), sizeof(bool));
        is.read(reinterpret_cast<char*>(&config.shadowDistanceCulling), 4 * 3);
        is.read(reinterpret_cast<char*>(&config.physicsBackend), sizeof(config.physicsBackend));
        is.read(reinterpret_cast<char*>(&config.physicsMode), sizeof(config.physicsMode));
        is.read(reinterpret_cast<char*>(&config.gravity), 4 * 3);
        is.read(reinterpret_cast<char*>(&config.maxSubSteps), 4 * 2);
        is.read(reinterpret_cast<char*>(&config.ccdEnabled), sizeof(bool));
        is.read(reinterpret_cast<char*>(&config.ccdThreshold), 4 * 2);
        is.read(reinterpret_cast<char*>(&config.mouseSensitivityX), 4 * 2);
        is.read(reinterpret_cast<char*>(&config.mouseInvertX), sizeof(bool) * 2);
        is.read(reinterpret_cast<char*>(&config.audioBackend), sizeof(config.audioBackend));
        is.read(reinterpret_cast<char*>(&config.masterVolume), 4);
        is.read(reinterpret_cast<char*>(&config.cullFaceEnabled), sizeof(bool) * 5);
        is.read(reinterpret_cast<char*>(&config.renderOrderEnabled), sizeof(bool));
        is.read(reinterpret_cast<char*>(&config.filterLayerMask), 4 * 2);

        configMgr.UpdateConfig(config);
    }

    std::vector<entt::entity> entities;
    std::vector<int32_t> parents;


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
            m.model = rm.GetModel(modelName);
            m.shader = rm.GetShader(shaderName);
            m.castShadow = castShadow;
        }

        bool hasMat;
        is.read(reinterpret_cast<char*>(&hasMat), sizeof(hasMat));
        if (hasMat)
        {
            auto& m = scene.registry.emplace<MaterialComponent>(entity);
            is.read(reinterpret_cast<char*>(&m.desc), sizeof(m.desc));
            m.gpu.dirty = true;
        }

        bool hasCam; is.read(reinterpret_cast<char*>(&hasCam), sizeof(hasCam));
        if (hasCam)
        {
            auto& c = scene.registry.emplace<CameraComponent>(entity);
            is.read(reinterpret_cast<char*>(&c), sizeof(CameraComponent));
        }

        bool hasDL; is.read(reinterpret_cast<char*>(&hasDL), sizeof(hasDL));
        if (hasDL)
        {
            auto& l = scene.registry.emplace<DirectionalLightComponent>(entity);
            is.read(reinterpret_cast<char*>(&l), sizeof(DirectionalLightComponent));
        }

        bool hasPL; is.read(reinterpret_cast<char*>(&hasPL), sizeof(hasPL));
        if (hasPL)
        {
            auto& l = scene.registry.emplace<PointLightComponent>(entity);
            is.read(reinterpret_cast<char*>(&l), sizeof(PointLightComponent));
        }

        bool hasSL; is.read(reinterpret_cast<char*>(&hasSL), sizeof(hasSL));
        if (hasSL)
        {
            auto& l = scene.registry.emplace<SpotLightComponent>(entity);
            is.read(reinterpret_cast<char*>(&l), sizeof(SpotLightComponent));
        }

        bool hasSky; is.read(reinterpret_cast<char*>(&hasSky), sizeof(hasSky));
        if (hasSky)
        {
            auto& s = scene.registry.emplace<SkyboxRenderComponent>(entity);
            is.read(reinterpret_cast<char*>(&s.isPrimary), sizeof(s.isPrimary));
            std::string sName = readString();
            std::string shName = readString();
            s.skybox = rm.GetSkybox(sName);
            s.shader = rm.GetShader(shName);
            if (s.isPrimary) EntityManager::SetActiveSkybox(scene, entity);
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
    LOGGER_INFO("BinarySceneSerializer") << "Deserialized scene: " << path;
    return true;
}




