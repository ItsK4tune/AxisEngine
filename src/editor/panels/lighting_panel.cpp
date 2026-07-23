#include <editor/panels/lighting_panel.h>

#ifdef ENABLE_EDITOR

#include <core/logic/config_manager.h>
#include <core/logic/filesystem.h>
#include <core/logic/service_locator.h>
#include <core/type/event_types.h>
#include <ecs/unit/light_components.h>
#include <ecs/unit/light_probe_components.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/reflection_components.h>
#include <editor/editor_system.h>
#include <resource/logic/resource_manager.h>
#include <resource/unit/model.h>
#include <scene/logic/scene.h>
#include <imgui.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>

namespace
{
struct BakeVertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

float Edge(glm::vec2 a, glm::vec2 b, glm::vec2 p)
{
    return (p.x - a.x) * (b.y - a.y) - (p.y - a.y) * (b.x - a.x);
}

std::string SafeAssetName(std::string name)
{
    for (char& value : name)
        if (!std::isalnum(static_cast<unsigned char>(value)) && value != '-' && value != '_')
            value = '_';
    return name.empty() ? "entity" : name;
}

std::unique_ptr<Model> LoadCpuModel(const MeshRendererComponent& renderer, ResourceManager& resources)
{
    if (renderer.model && !renderer.model->meshes.empty() &&
        renderer.model->meshes.front().HasCpuVertexData())
        return {};
    for (const auto& definition : resources.GetResourceDefinitions())
    {
        if (definition.type != "Model" || definition.name != renderer.modelName)
            continue;
        const auto path = definition.properties.find("Path");
        if (path == definition.properties.end())
            return {};
        auto model = std::make_unique<Model>();
        model->LoadCPU(FileSystem::getPath(path->second), true);
        return model;
    }
    return {};
}
}

void LightingPanel::OnImGui(Scene& scene)
{
    ImGui::Begin(GetTitle().c_str(), &m_Open);
    auto* configManager = ServiceLocator::Instance().Resolve<ConfigManager>();
    if (configManager)
    {
        auto config = configManager->GetConfig();
        const char* modes[] = {"Bake", "Light probes", "Reflection probes", "Real time"};
        int mode = static_cast<int>(config.lightingMode);
        if (ImGui::Combo("Lighting mode", &mode, modes, IM_ARRAYSIZE(modes)))
        {
            config.lightingMode = static_cast<LightingMode>(mode);
            configManager->UpdateConfig(config, ConfigChangedEvent::Graphics);
        }
    }

    const auto directional = scene.View<DirectionalLightComponent>().size();
    const auto points = scene.View<PointLightComponent>().size();
    const auto spots = scene.View<SpotLightComponent>().size();
    const auto lightProbes = scene.View<LightProbeComponent>().size();
    const auto reflectionProbes = scene.View<ReflectionProbeComponent>().size();
    ImGui::Text("Directional: %zu", directional);
    ImGui::Text("Point: %zu", points);
    ImGui::Text("Spot: %zu", spots);
    ImGui::Text("Light probes: %zu", lightProbes);
    ImGui::Text("Reflection probes: %zu", reflectionProbes);

    ImGui::SeparatorText("Static lightmaps");
    ImGui::InputText("Output image path", m_OutputPath.data(), m_OutputPath.size());
    ImGui::TextDisabled("Use {entity} to create one image per static mesh, for example lightmaps/{entity}.ppm.");
    ImGui::SliderInt("Resolution", &m_LightmapResolution, 32, 1024);
    ImGui::SliderFloat("Baked ambient", &m_Ambient, 0.0f, 1.0f);
    if (ImGui::Button("Bake static mesh lightmaps"))
        m_Status = BakeLightmaps(scene) ? "Lightmaps baked and assigned to static mesh materials."
                                        : "No compatible static mesh with UVs could be baked.";
    ImGui::TextWrapped("The baker rasterizes existing mesh UVs and stores ambient plus direct directional/point "
                       "lighting. Models should use non-overlapping UVs for deterministic results.");

    if (ImGui::Button("Refresh static reflection probes"))
    {
        size_t refreshed = 0;
        auto probes = scene.View<ReflectionProbeComponent>();
        for (const entt::entity entity : probes)
        {
            auto& probe = probes.get<ReflectionProbeComponent>(entity);
            if (probe.type == ReflectionProbeType::Static)
            {
                probe.isDirty = true;
                probe.currentFace = 0;
                ++refreshed;
            }
        }
        m_Status = "Queued " + std::to_string(refreshed) + " static probes for incremental capture.";
    }
    ImGui::TextWrapped("Static probes are captured through the existing per-frame face budget, avoiding a long "
                       "blocking bake on the UI thread.");
    if (!m_Status.empty())
        ImGui::TextWrapped("%s", m_Status.c_str());
    ImGui::End();
}

bool LightingPanel::BakeLightmaps(Scene& scene)
{
    auto* resources = ServiceLocator::Instance().Resolve<ResourceManager>();
    if (!resources)
        return false;
    const int resolution = std::clamp(m_LightmapResolution, 32, 1024);
    const std::filesystem::path requestedPath = std::filesystem::path(m_OutputPath.data()).lexically_normal();
    if (requestedPath.empty())
        return false;
    const std::string requestedText = requestedPath.generic_string();
    const bool hasEntityToken = requestedText.find("{entity}") != std::string::npos;

    struct Directional
    {
        glm::vec3 direction;
        glm::vec3 color;
        float intensity;
    };
    std::vector<Directional> directionalLights;
    auto directionalView = scene.View<DirectionalLightComponent>();
    for (const entt::entity entity : directionalView)
    {
        const auto& light = directionalView.get<DirectionalLightComponent>(entity);
        if (light.active)
            directionalLights.push_back({glm::normalize(light.direction), light.color,
                                         light.intensity * light.diffuse});
    }

    struct Point
    {
        glm::vec3 position;
        glm::vec3 color;
        float intensity;
        float radius;
    };
    std::vector<Point> pointLights;
    auto pointView = scene.View<PointLightComponent, WorldTransformComponent>();
    for (const entt::entity entity : pointView)
    {
        const auto& light = pointView.get<PointLightComponent>(entity);
        if (light.active)
            pointLights.push_back({glm::vec3(pointView.get<WorldTransformComponent>(entity).worldMatrix[3]),
                                   light.color, light.intensity * light.diffuse, light.radius});
    }

    size_t baked = 0;
    EditorSystem::BeginTransaction(scene, "Bake lightmaps");
    auto renderables = scene.View<MeshRendererComponent, MaterialComponent, WorldTransformComponent>();
    for (const entt::entity entity : renderables)
    {
        auto& renderer = renderables.get<MeshRendererComponent>(entity);
        if (!renderer.model || !renderer.model->IsStatic())
            continue;
        std::unique_ptr<Model> cpuModel = LoadCpuModel(renderer, *resources);
        Model* model = cpuModel ? cpuModel.get() : renderer.model.get();
        if (!model)
            continue;

        std::vector<glm::vec3> pixels(static_cast<size_t>(resolution * resolution), glm::vec3(m_Ambient));
        std::vector<uint8_t> covered(pixels.size(), 0);
        const glm::mat4 world = renderables.get<WorldTransformComponent>(entity).worldMatrix;
        const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(world)));
        bool wroteTriangle = false;

        for (const Mesh& mesh : model->meshes)
        {
            if (!mesh.HasCpuVertexData() || mesh.indices.size() < 3)
                continue;
            const auto readVertex = [&](size_t index) {
                BakeVertex result{};
                const uint8_t* source = mesh.m_VertexData.data() + index * mesh.m_VertexStride;
                if (mesh.m_IsSkinned)
                {
                    const auto& vertex = *reinterpret_cast<const SkinnedVertex*>(source);
                    result = {vertex.Position, vertex.Normal, vertex.TexCoords};
                }
                else
                {
                    const auto& vertex = *reinterpret_cast<const StaticVertex*>(source);
                    result = {vertex.Position, vertex.Normal, vertex.TexCoords};
                }
                result.position = glm::vec3(world * glm::vec4(result.position, 1.0f));
                result.normal = glm::normalize(normalMatrix * result.normal);
                result.uv.y = 1.0f - result.uv.y;
                return result;
            };

            for (size_t index = 0; index + 2 < mesh.indices.size(); index += 3)
            {
                const BakeVertex a = readVertex(mesh.indices[index]);
                const BakeVertex b = readVertex(mesh.indices[index + 1]);
                const BakeVertex c = readVertex(mesh.indices[index + 2]);
                const glm::vec2 pa = a.uv * static_cast<float>(resolution - 1);
                const glm::vec2 pb = b.uv * static_cast<float>(resolution - 1);
                const glm::vec2 pc = c.uv * static_cast<float>(resolution - 1);
                const float area = Edge(pa, pb, pc);
                if (std::abs(area) < 0.0001f)
                    continue;
                const int minX = std::clamp(static_cast<int>(std::floor(std::min({pa.x, pb.x, pc.x}))), 0,
                                            resolution - 1);
                const int maxX = std::clamp(static_cast<int>(std::ceil(std::max({pa.x, pb.x, pc.x}))), 0,
                                            resolution - 1);
                const int minY = std::clamp(static_cast<int>(std::floor(std::min({pa.y, pb.y, pc.y}))), 0,
                                            resolution - 1);
                const int maxY = std::clamp(static_cast<int>(std::ceil(std::max({pa.y, pb.y, pc.y}))), 0,
                                            resolution - 1);
                for (int y = minY; y <= maxY; ++y)
                {
                    for (int x = minX; x <= maxX; ++x)
                    {
                        const glm::vec2 p(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f);
                        const float wa = Edge(pb, pc, p) / area;
                        const float wb = Edge(pc, pa, p) / area;
                        const float wc = 1.0f - wa - wb;
                        if (wa < 0.0f || wb < 0.0f || wc < 0.0f)
                            continue;
                        const glm::vec3 normal = glm::normalize(a.normal * wa + b.normal * wb + c.normal * wc);
                        const glm::vec3 position = a.position * wa + b.position * wb + c.position * wc;
                        glm::vec3 irradiance(m_Ambient);
                        for (const auto& light : directionalLights)
                            irradiance += light.color * light.intensity *
                                          std::max(glm::dot(normal, -light.direction), 0.0f);
                        for (const auto& light : pointLights)
                        {
                            const glm::vec3 delta = light.position - position;
                            const float distance = glm::length(delta);
                            if (distance < light.radius && distance > 0.0001f)
                            {
                                const float falloff = 1.0f - distance / light.radius;
                                irradiance += light.color * light.intensity * falloff * falloff *
                                              std::max(glm::dot(normal, delta / distance), 0.0f);
                            }
                        }
                        const size_t pixel = static_cast<size_t>(y * resolution + x);
                        pixels[pixel] = glm::clamp(irradiance, glm::vec3(0.0f), glm::vec3(1.0f));
                        covered[pixel] = 1;
                        wroteTriangle = true;
                    }
                }
            }
        }
        if (!wroteTriangle)
            continue;

        std::string entityName = std::to_string(static_cast<uint32_t>(entt::to_entity(entity)));
        if (const auto* info = scene.TryGetComponent<InfoComponent>(entity))
            entityName = SafeAssetName(info->name) + "_" + entityName;
        std::string outputText = requestedText;
        if (hasEntityToken)
            outputText.replace(outputText.find("{entity}"), 8, entityName);
        else
        {
            std::filesystem::path base(outputText);
            if (!base.has_extension())
                base /= entityName + ".ppm";
            else
                base = base.parent_path() / (base.stem().string() + "_" + entityName + base.extension().string());
            outputText = base.generic_string();
        }
        std::filesystem::path path(outputText);
        if (path.extension() != ".ppm")
            path.replace_extension(".ppm");
        std::error_code error;
        if (!path.parent_path().empty())
            std::filesystem::create_directories(path.parent_path(), error);
        if (error)
            continue;
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        if (!output)
            continue;
        output << "P6\n" << resolution << ' ' << resolution << "\n255\n";
        for (const glm::vec3 color : pixels)
        {
            const unsigned char rgb[] = {
                static_cast<unsigned char>(std::round(color.r * 255.0f)),
                static_cast<unsigned char>(std::round(color.g * 255.0f)),
                static_cast<unsigned char>(std::round(color.b * 255.0f))};
            output.write(reinterpret_cast<const char*>(rgb), sizeof(rgb));
        }
        output.close();

        auto& material = renderables.get<MaterialComponent>(entity);
        material.desc.lightmapPath = path.generic_string();
        material.desc.lightmapIntensity = 1.0f;
        material.gpu.lightmapMap = 0;
        material.gpu.dirty = true;
        material.gpu.batchKeyDirty = true;
        resources->LoadTexture(material.desc.lightmapPath, material.desc.lightmapPath, false);
        ++baked;
    }
    return baked != 0;
}

#endif
