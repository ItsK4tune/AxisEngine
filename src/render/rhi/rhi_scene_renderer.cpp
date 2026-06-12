#include <render/rhi/rhi_scene_renderer.h>
#include <core/logic/filesystem.h>
#include <core/logic/logger.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/decal_component.h>
#include <ecs/unit/light_components.h>
#include <render/rhi/i_command_list.h>
#include <render/rhi/i_render_backend.h>
#include <resource/unit/mesh.h>
#include <resource/unit/model.h>
#include <scene/logic/scene.h>
#include <entt/entt.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <system_error>

#ifndef AXIS_SHADER_ROOT_DIR
#define AXIS_SHADER_ROOT_DIR ""
#endif

namespace
{
std::string FindDxcPath()
{
    if (const char* sdkPath = std::getenv("VULKAN_SDK"))
    {
        std::filesystem::path p(sdkPath);
        p /= "Bin/dxc.exe";
        if (std::filesystem::exists(p))
            return p.string();
    }
    std::string defaults[] = {"C:/VulkanSDK/1.4.350.0/Bin/dxc.exe", "C:/VulkanSDK/1.3.290.0/Bin/dxc.exe",
                              "C:/VulkanSDK/1.3.283.0/Bin/dxc.exe", "C:/VulkanSDK/1.3.275.0/Bin/dxc.exe",
                              "C:/VulkanSDK/1.3.268.0/Bin/dxc.exe", "C:/VulkanSDK/1.3.250.1/Bin/dxc.exe"};
    for (const auto& d : defaults)
    {
        if (std::filesystem::exists(d))
            return d;
    }
    return "dxc.exe";
}

bool CompileHLSLShader(const std::string& hlslPath, const std::string& entryPoint, rhi::ShaderStage stage,
                       const std::string& outputPath, rhi::BackendType backend)
{
    namespace fs = std::filesystem;
    if (!fs::exists(hlslPath))
    {
        LOGGER_ERROR("ShaderCompiler") << "HLSL source file does not exist: " << hlslPath;
        return false;
    }

    std::error_code ec;
    fs::create_directories(fs::path(outputPath).parent_path(), ec);

    bool needCompile = true;
    if (fs::exists(outputPath))
    {
        auto hlslTime = fs::last_write_time(hlslPath);
        auto outTime = fs::last_write_time(outputPath);
        if (outTime >= hlslTime)
        {
            needCompile = false;
        }
    }

    if (!needCompile)
    {
        return true;
    }

    std::string dxc = FindDxcPath();
    std::string target = (stage == rhi::ShaderStage::Vertex) ? "vs_6_0" : "ps_6_0";

    std::string cmd;
    if (backend == rhi::BackendType::Vulkan)
    {
        cmd = "\"" + dxc + "\" -HV 2021 -T " + target + " -E " + entryPoint +
              " -spirv -fspv-target-env=vulkan1.2 -DAXIS_VULKAN=1 -Fo \"" + outputPath + "\" \"" + hlslPath + "\"";
    }
    else
    {
        cmd = "\"" + dxc + "\" -HV 2021 -T " + target + " -E " + entryPoint + " -Fo \"" + outputPath + "\" \"" +
              hlslPath + "\"";
    }

    LOGGER_INFO("ShaderCompiler") << "Compiling HLSL: " << cmd;
    int result = std::system(cmd.c_str());
    if (result != 0)
    {
        LOGGER_ERROR("ShaderCompiler") << "Failed to compile HLSL shader: " << hlslPath << " (Error code: " << result
                                       << ")";
        return false;
    }

    return fs::exists(outputPath);
}

std::string ShaderAssetPath(const std::string& relativePath)
{
    std::filesystem::path root = AXIS_SHADER_ROOT_DIR;
    if (!root.empty())
        return (root / relativePath).string();
    return FileSystem::getPath((std::filesystem::path("include/engine/asset/shaders") / relativePath).string());
}

glm::mat4 MakeClipSpaceProjection(glm::mat4 projection, rhi::BackendType backend)
{
    if (backend == rhi::BackendType::OpenGL)
        return projection;

    glm::mat4 clip(1.0f);
    if (backend == rhi::BackendType::Vulkan)
        clip[1][1] = -1.0f;
    clip[2][2] = 0.5f;
    clip[3][2] = 0.5f;
    return clip * projection;
}

glm::vec4 ResolveColor(Scene& scene, entt::entity entity, const MeshRendererComponent& renderer)
{
    glm::vec4 color = renderer.color;
    if (auto* material = scene.TryGetComponent<MaterialComponent>(entity))
        color.a *= material->desc.opacity;
    return color;
}

struct SkyboxPushConstants
{
    glm::mat4 mvp;
    float intensity;
    float pad[3];
};

struct PbrPushConstants
{
    glm::mat4 mvp;
    glm::mat4 model;
    glm::vec4 color;
    glm::vec4 pbrParams;  // x = roughness, y = metallic, z = ao, w = unused
    glm::vec4 cameraPos;
    glm::vec4 dirLightDir;    // direction (xyz) + active (w)
    glm::vec4 dirLightColor;  // color (rgb) + intensity (w)
};

struct DecalPushConstants
{
    glm::mat4 mvp;
    glm::mat4 model;
    glm::vec4 tintColor;
    glm::vec4 decalParams;  // x = roughness, y = metallic, z = reflectivity, w = lightingMode
};
}  // namespace

RhiSceneRenderer::RhiSceneRenderer(rhi::IRenderBackend& backend) : m_Backend(backend)
{
}

RhiSceneRenderer::~RhiSceneRenderer()
{
    Shutdown();
}

bool RhiSceneRenderer::Render(Scene& scene, int width, int height, float alpha)
{
    (void)alpha;
    LOGGER_INFO("Render") << "Render call started";

    auto& swapchain = m_Backend.GetSwapchain();
    rhi::ImageHandle backBuffer = swapchain.GetCurrentBackBuffer();
    const bool usesDefaultBackBuffer = m_Backend.GetBackendType() == rhi::BackendType::OpenGL;
    if (!backBuffer && !usesDefaultBackBuffer)
    {
        LOGGER_INFO("Render") << "No backbuffer, return false";
        return false;
    }

    rhi::Extent2D extent = swapchain.GetExtent();
    if (width > 0)
        extent.width = static_cast<uint32_t>(width);
    if (height > 0)
        extent.height = static_cast<uint32_t>(height);
    if (extent.width == 0 || extent.height == 0)
    {
        LOGGER_INFO("Render") << "Invalid extent, return false";
        return false;
    }

    LOGGER_INFO("Render") << "Ensuring resources";
    if (!EnsureResources(extent.width, extent.height))
    {
        LOGGER_INFO("Render") << "EnsureResources failed, return false";
        return false;
    }

    LOGGER_INFO("Render") << "Resolving active camera";
    entt::entity cameraEntity = scene.GetActiveCamera();
    if (cameraEntity == entt::null || !scene.HasAllComponents<CameraComponent>(cameraEntity))
    {
        LOGGER_INFO("Render") << "No active camera, return false";
        return false;
    }

    const auto& camera = scene.GetComponent<CameraComponent>(cameraEntity);
    const glm::mat4 projection = MakeClipSpaceProjection(camera.projectionMatrix, m_Backend.GetBackendType());
    const glm::mat4 viewProjection = projection * camera.viewMatrix;
    const glm::vec3 cameraPos = glm::vec3(glm::inverse(camera.viewMatrix)[3]);

    // Find active directional light
    glm::vec4 dirLightDirVec = glm::vec4(0.0f);
    glm::vec4 dirLightColorVec = glm::vec4(0.0f);
    auto dirLightView = scene.View<DirectionalLightComponent>();
    for (auto entity : dirLightView)
    {
        const auto& light = dirLightView.get<DirectionalLightComponent>(entity);
        if (light.active)
        {
            dirLightDirVec = glm::vec4(light.direction, 1.0f);
            dirLightColorVec = glm::vec4(light.color, light.intensity);
            break;
        }
    }

    LOGGER_INFO("Render") << "Beginning command list";
    auto& device = m_Backend.GetDevice();
    auto& commandList = device.BeginCommandList(rhi::CommandQueueType::Graphics);

    rhi::RenderAttachmentDesc colorAttachment;
    colorAttachment.image = backBuffer;
    colorAttachment.format = swapchain.GetBackBufferFormat();
    colorAttachment.loadOp = rhi::LoadOp::Load;
    colorAttachment.storeOp = rhi::StoreOp::Store;

    rhi::DepthStencilAttachmentDesc depthAttachment;
    depthAttachment.image = m_DepthImage;
    depthAttachment.format = rhi::Format::Depth32F;
    depthAttachment.depthLoadOp = rhi::LoadOp::Clear;
    depthAttachment.depthStoreOp = rhi::StoreOp::Store;
    depthAttachment.clearValue = {1.0f, 0};

    rhi::RenderPassBeginInfo beginInfo;
    beginInfo.colorAttachments.push_back(colorAttachment);
    beginInfo.hasDepthStencilAttachment = true;
    beginInfo.depthStencilAttachment = depthAttachment;
    beginInfo.renderArea = {0, 0, extent.width, extent.height};

    commandList.BeginRendering(beginInfo);
    commandList.SetViewport(
        rhi::Viewport{0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f});
    commandList.SetScissor(rhi::Rect2D{0, 0, extent.width, extent.height});

    uint32_t totalEntities = 0;
    uint32_t activeEntities = 0;
    uint32_t modelEntities = 0;
    uint32_t readyMeshes = 0;
    uint32_t drawCalls = 0;

    // 1. Render Skybox
    LOGGER_INFO("Render") << "Rendering skybox";
    entt::entity skyboxEntity = scene.GetActiveSkybox();
    if (skyboxEntity != entt::null && scene.HasAllComponents<SkyboxRenderComponent>(skyboxEntity))
    {
        rhi::PipelineHandle skyboxPipeline = GetOrCreateSkyboxPipeline();
        if (skyboxPipeline && m_SkyboxVbo)
        {
            SkyboxPushConstants push;
            glm::mat4 skyboxView = glm::mat4(glm::mat3(camera.viewMatrix));
            push.mvp = glm::transpose(projection * skyboxView);
            push.intensity = 1.0f;  // Default skybox intensity fallback

            commandList.BindPipeline(skyboxPipeline);
            commandList.BindVertexBuffer(0, m_SkyboxVbo);
            commandList.PushConstants(rhi::ShaderStage::Vertex | rhi::ShaderStage::Fragment, &push, sizeof(push));
            commandList.Draw(36);
            ++drawCalls;
            LOGGER_INFO("Render") << "Skybox draw submitted";
        }
    }

    // 2. Render Opaque / Lit / Unlit Meshes
    LOGGER_INFO("Render") << "Rendering opaque meshes";
    auto renderView = scene.View<WorldTransformComponent, MeshRendererComponent, InfoComponent>();
    for (auto entity : renderView)
    {
        ++totalEntities;
        const auto& info = renderView.get<InfoComponent>(entity);
        if (!info.isActive)
            continue;
        ++activeEntities;

        const auto& renderer = renderView.get<MeshRendererComponent>(entity);
        if (!renderer.model)
            continue;

        ++modelEntities;
        Model& model = *renderer.model;
        if (!model.IsReadyToRender())
        {
            LOGGER_INFO("Render") << "Uploading model to GPU for entity " << static_cast<uint32_t>(entity);
            model.UploadToGPU();
            LOGGER_INFO("Render") << "Model uploaded";
        }

        const auto& world = renderView.get<WorldTransformComponent>(entity);
        const glm::mat4 modelMatrix = world.worldMatrix * model.GetRootTransform();
        const glm::vec4 color = ResolveColor(scene, entity, renderer);

        // Check if there is a material component
        auto* material = scene.TryGetComponent<MaterialComponent>(entity);
        const bool isPbr = material && (material->desc.type == "PBR");

        for (auto& mesh : model.meshes)
        {
            if (!mesh.IsInitialized())
            {
                LOGGER_INFO("Render") << "Initializing mesh for entity " << static_cast<uint32_t>(entity);
                mesh.setupMesh();
                LOGGER_INFO("Render") << "Mesh initialized";
            }
            if (!mesh.GetRhiVbo() || !mesh.GetRhiEbo() || mesh.indices.empty())
                continue;

            ++readyMeshes;

            if (isPbr)
            {
                rhi::PipelineHandle pipeline = GetOrCreateLitPipeline(static_cast<uint32_t>(mesh.m_VertexStride));
                if (!pipeline)
                    continue;

                PbrPushConstants push;
                push.mvp = glm::transpose(viewProjection * modelMatrix);
                push.model = glm::transpose(modelMatrix);
                push.color = color;
                push.pbrParams =
                    glm::vec4(material->desc.pbr.roughness, material->desc.pbr.metallic, material->desc.pbr.ao, 1.0f);
                push.cameraPos = glm::vec4(cameraPos, 1.0f);
                push.dirLightDir = dirLightDirVec;
                push.dirLightColor = dirLightColorVec;

                commandList.BindPipeline(pipeline);
                commandList.BindVertexBuffer(0, mesh.GetRhiVbo());
                commandList.BindIndexBuffer(mesh.GetRhiEbo(), rhi::IndexType::UInt32);
                commandList.PushConstants(rhi::ShaderStage::Vertex | rhi::ShaderStage::Fragment, &push, sizeof(push));
                commandList.DrawIndexed(static_cast<uint32_t>(mesh.indices.size()));
            }
            else
            {
                LOGGER_INFO("Render") << "Binding Unlit pipeline for stride " << mesh.m_VertexStride;
                rhi::PipelineHandle pipeline = GetOrCreatePipeline(static_cast<uint32_t>(mesh.m_VertexStride));
                if (!pipeline)
                    continue;

                PushConstants push;
                push.mvp = glm::transpose(viewProjection * modelMatrix);
                push.color = color;

                commandList.BindPipeline(pipeline);
                commandList.BindVertexBuffer(0, mesh.GetRhiVbo());
                commandList.BindIndexBuffer(mesh.GetRhiEbo(), rhi::IndexType::UInt32);
                commandList.PushConstants(rhi::ShaderStage::Vertex | rhi::ShaderStage::Fragment, &push, sizeof(push));
                commandList.DrawIndexed(static_cast<uint32_t>(mesh.indices.size()));
            }
            ++drawCalls;
        }
    }

    // 3. Render Decals
    LOGGER_INFO("Render") << "Rendering decals";
    auto decalView = scene.View<DecalComponent, PositionComponent>();
    rhi::PipelineHandle decalPipeline = GetOrCreateDecalPipeline(32);  // Stride 32 for our quad layout
    if (decalPipeline && m_DecalQuadVbo)
    {
        for (auto entity : decalView)
        {
            const auto& decal = decalView.get<DecalComponent>(entity);
            const auto& pos = decalView.get<PositionComponent>(entity);
            auto* rotComp = scene.TryGetComponent<RotationComponent>(entity);
            auto* scaleComp = scene.TryGetComponent<ScaleComponent>(entity);

            glm::quat rotation = rotComp ? rotComp->value : glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            glm::vec3 scale = scaleComp ? scaleComp->value : glm::vec3(1.0f);

            glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), pos.value) * glm::mat4_cast(rotation) *
                                    glm::scale(glm::mat4(1.0f), scale);
            // Match legacy forward scale factor
            modelMatrix = glm::scale(modelMatrix, glm::vec3(0.5f));

            DecalPushConstants push;
            push.mvp = glm::transpose(viewProjection * modelMatrix);
            push.model = glm::transpose(modelMatrix);
            push.tintColor = decal.tintColor * decal.opacity;
            push.decalParams =
                glm::vec4(decal.roughness, decal.metallic, decal.reflectivity, static_cast<float>(decal.lightingMode));

            LOGGER_INFO("Render") << "Submitting decal draw call";
            commandList.BindPipeline(decalPipeline);
            commandList.BindVertexBuffer(0, m_DecalQuadVbo);
            commandList.PushConstants(rhi::ShaderStage::Vertex | rhi::ShaderStage::Fragment, &push, sizeof(push));
            commandList.Draw(6);
            ++drawCalls;
        }
    }

    LOGGER_INFO("Render") << "Ending rendering and submitting command list";

    commandList.EndRendering();
    device.Submit(commandList);

    if (!m_FirstFrameLogged && (totalEntities > 0 || modelEntities > 0 || drawCalls > 0))
    {
        LOGGER_INFO("RhiSceneRenderer") << "Native scene frame: total=" << totalEntities << " active=" << activeEntities
                                        << " modelEntities=" << modelEntities << " readyMeshes=" << readyMeshes
                                        << " drawCalls=" << drawCalls << " extent=" << extent.width << "x"
                                        << extent.height;
        m_FirstFrameLogged = true;
    }

    return true;
}

void RhiSceneRenderer::OnResize(uint32_t width, uint32_t height)
{
    if (width == m_DepthWidth && height == m_DepthHeight)
        return;

    auto& device = m_Backend.GetDevice();
    if (m_DepthImage)
    {
        device.DestroyImage(m_DepthImage);
        m_DepthImage = {};
    }
    m_DepthWidth = 0;
    m_DepthHeight = 0;
}

void RhiSceneRenderer::Shutdown()
{
    auto& device = m_Backend.GetDevice();
    DestroyPipelines();

    if (m_VertexShader)
    {
        device.DestroyShaderModule(m_VertexShader);
        m_VertexShader = {};
    }
    if (m_FragmentShader)
    {
        device.DestroyShaderModule(m_FragmentShader);
        m_FragmentShader = {};
    }
    if (m_SkyboxVS)
    {
        device.DestroyShaderModule(m_SkyboxVS);
        m_SkyboxVS = {};
    }
    if (m_SkyboxPS)
    {
        device.DestroyShaderModule(m_SkyboxPS);
        m_SkyboxPS = {};
    }
    if (m_LitVS)
    {
        device.DestroyShaderModule(m_LitVS);
        m_LitVS = {};
    }
    if (m_LitPS)
    {
        device.DestroyShaderModule(m_LitPS);
        m_LitPS = {};
    }
    if (m_DecalVS)
    {
        device.DestroyShaderModule(m_DecalVS);
        m_DecalVS = {};
    }
    if (m_DecalPS)
    {
        device.DestroyShaderModule(m_DecalPS);
        m_DecalPS = {};
    }

    if (m_DepthImage)
    {
        device.DestroyImage(m_DepthImage);
        m_DepthImage = {};
    }
    if (m_SkyboxVbo)
    {
        device.DestroyBuffer(m_SkyboxVbo);
        m_SkyboxVbo = {};
    }
    if (m_DecalQuadVbo)
    {
        device.DestroyBuffer(m_DecalQuadVbo);
        m_DecalQuadVbo = {};
    }

    m_DepthWidth = 0;
    m_DepthHeight = 0;
}

bool RhiSceneRenderer::EnsureResources(uint32_t width, uint32_t height)
{
    if (!EnsureShaders())
        return false;

    auto& device = m_Backend.GetDevice();

    if (!m_SkyboxVbo)
    {
        float skyboxVertices[] = {-1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
                                  1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

                                  -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
                                  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

                                  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
                                  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

                                  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
                                  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

                                  -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
                                  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

                                  -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
                                  1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f};
        rhi::BufferDesc vboDesc;
        vboDesc.size = sizeof(skyboxVertices);
        vboDesc.usage = rhi::BufferUsage::Vertex;
        vboDesc.memoryUsage = rhi::MemoryUsage::GpuOnly;
        m_SkyboxVbo = device.CreateBuffer(vboDesc, skyboxVertices);
    }

    if (!m_DecalQuadVbo)
    {
        float decalQuadVertices[] = {-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f,  0.0f, 1.0f, -0.5f, -0.5f, 0.0f, 0.0f,
                                     0.0f,  1.0f, 0.0f, 0.0f, 0.5f, -0.5f, 0.0f, 0.0f, 0.0f,  1.0f,  1.0f, 0.0f,

                                     -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f,  0.0f, 1.0f, 0.5f,  -0.5f, 0.0f, 0.0f,
                                     0.0f,  1.0f, 1.0f, 0.0f, 0.5f, 0.5f,  0.0f, 0.0f, 0.0f,  1.0f,  1.0f, 1.0f};
        rhi::BufferDesc vboDesc;
        vboDesc.size = sizeof(decalQuadVertices);
        vboDesc.usage = rhi::BufferUsage::Vertex;
        vboDesc.memoryUsage = rhi::MemoryUsage::GpuOnly;
        m_DecalQuadVbo = device.CreateBuffer(vboDesc, decalQuadVertices);
    }

    if (m_DepthImage && m_DepthWidth == width && m_DepthHeight == height)
        return true;

    if (m_DepthImage)
        device.DestroyImage(m_DepthImage);

    rhi::ImageDesc depthDesc;
    depthDesc.width = width;
    depthDesc.height = height;
    depthDesc.format = rhi::Format::Depth32F;
    depthDesc.usage = rhi::ImageUsage::DepthStencilAttachment;
    depthDesc.debugName = "RHI Scene Depth";
    m_DepthImage = device.CreateImage(depthDesc);
    if (!m_DepthImage)
        return false;

    m_DepthWidth = width;
    m_DepthHeight = height;
    return true;
}

bool RhiSceneRenderer::EnsureShaders()
{
    if (m_VertexShader && m_FragmentShader && m_SkyboxVS && m_SkyboxPS && m_LitVS && m_LitPS && m_DecalVS && m_DecalPS)
        return true;
    if (m_ShaderLoadFailed)
        return false;

    const rhi::BackendType backend = m_Backend.GetBackendType();

    auto createModule = [&](const std::vector<uint8_t>& bytes, rhi::ShaderStage stage, rhi::ShaderSourceType sourceType,
                            const char* entryPoint, const char* debugName) -> rhi::ShaderModuleHandle {
        if (bytes.empty())
        {
            m_ShaderLoadFailed = true;
            return {};
        }
        rhi::ShaderModuleDesc desc;
        desc.stage = stage;
        desc.sourceType = sourceType;
        desc.data = bytes.data();
        desc.size = bytes.size();
        desc.entryPoint = entryPoint;
        desc.debugName = debugName;
        return m_Backend.GetDevice().CreateShaderModule(desc);
    };

    if (backend == rhi::BackendType::OpenGL)
    {
        auto loadGlsl = [&](const char* file, rhi::ShaderStage stage,
                            const char* debugName) -> rhi::ShaderModuleHandle {
            std::vector<uint8_t> bytes = LoadShaderFile(std::string("opengl/rhi/") + file);
            return createModule(bytes, stage, rhi::ShaderSourceType::SourceText, "main", debugName);
        };

        m_VertexShader = loadGlsl("rhi_forward_unlit.vert.glsl", rhi::ShaderStage::Vertex, "RHI Forward Unlit VS");
        m_FragmentShader = loadGlsl("rhi_forward_unlit.frag.glsl", rhi::ShaderStage::Fragment, "RHI Forward Unlit PS");

        m_SkyboxVS = loadGlsl("rhi_skybox.vert.glsl", rhi::ShaderStage::Vertex, "RHI Skybox VS");
        m_SkyboxPS = loadGlsl("rhi_skybox.frag.glsl", rhi::ShaderStage::Fragment, "RHI Skybox PS");

        m_LitVS = loadGlsl("rhi_pbr_lit.vert.glsl", rhi::ShaderStage::Vertex, "RHI Lit VS");
        m_LitPS = loadGlsl("rhi_pbr_lit.frag.glsl", rhi::ShaderStage::Fragment, "RHI Lit PS");

        m_DecalVS = loadGlsl("rhi_decal.vert.glsl", rhi::ShaderStage::Vertex, "RHI Decal VS");
        m_DecalPS = loadGlsl("rhi_decal.frag.glsl", rhi::ShaderStage::Fragment, "RHI Decal PS");
    }
    else
    {
        const bool isVulkan = backend == rhi::BackendType::Vulkan;
        const rhi::ShaderSourceType sourceType =
            isVulkan ? rhi::ShaderSourceType::SpirVBinary : rhi::ShaderSourceType::DxilBinary;
        const char* backendDir = isVulkan ? "vulkan/rhi/" : "directx/rhi/";

        auto loadCompiledHlsl = [&](const char* sourceBase, const char* spirvFile, const char* dxilFile,
                                    rhi::ShaderStage stage, const char* debugName) -> rhi::ShaderModuleHandle {
            const char* binaryFile = isVulkan ? spirvFile : dxilFile;
            const std::string relativeOutput = std::string(backendDir) + binaryFile;
            const std::string hlslPath = ShaderAssetPath(std::string("shared/rhi/") + sourceBase + ".hlsl");
            const std::string outputPath = ShaderAssetPath(relativeOutput);
            const std::string entry = (stage == rhi::ShaderStage::Vertex) ? "VSMain" : "PSMain";

            if (!CompileHLSLShader(hlslPath, entry, stage, outputPath, backend))
            {
                m_ShaderLoadFailed = true;
                return {};
            }

            std::vector<uint8_t> bytes = LoadShaderFile(relativeOutput);
            return createModule(bytes, stage, sourceType, entry.c_str(), debugName);
        };

        m_VertexShader = loadCompiledHlsl("rhi_forward_unlit", "rhi_forward_unlit.vs.spv", "rhi_forward_unlit.vs.dxil",
                                          rhi::ShaderStage::Vertex, "RHI Forward Unlit VS");
        m_FragmentShader =
            loadCompiledHlsl("rhi_forward_unlit", "rhi_forward_unlit.ps.spv", "rhi_forward_unlit.ps.dxil",
                             rhi::ShaderStage::Fragment, "RHI Forward Unlit PS");

        m_SkyboxVS = loadCompiledHlsl("rhi_skybox", "rhi_skybox.vs.spv", "rhi_skybox.vs.dxil", rhi::ShaderStage::Vertex,
                                      "RHI Skybox VS");
        m_SkyboxPS = loadCompiledHlsl("rhi_skybox", "rhi_skybox.ps.spv", "rhi_skybox.ps.dxil",
                                      rhi::ShaderStage::Fragment, "RHI Skybox PS");

        m_LitVS = loadCompiledHlsl("rhi_pbr_lit", "rhi_pbr_lit.vs.spv", "rhi_pbr_lit.vs.dxil", rhi::ShaderStage::Vertex,
                                   "RHI Lit VS");
        m_LitPS = loadCompiledHlsl("rhi_pbr_lit", "rhi_pbr_lit.ps.spv", "rhi_pbr_lit.ps.dxil",
                                   rhi::ShaderStage::Fragment, "RHI Lit PS");

        m_DecalVS = loadCompiledHlsl("rhi_decal", "rhi_decal.vs.spv", "rhi_decal.vs.dxil", rhi::ShaderStage::Vertex,
                                     "RHI Decal VS");
        m_DecalPS = loadCompiledHlsl("rhi_decal", "rhi_decal.ps.spv", "rhi_decal.ps.dxil", rhi::ShaderStage::Fragment,
                                     "RHI Decal PS");
    }

    if (!m_VertexShader || !m_FragmentShader || !m_SkyboxVS || !m_SkyboxPS || !m_LitVS || !m_LitPS || !m_DecalVS ||
        !m_DecalPS)
    {
        m_ShaderLoadFailed = true;
        LOGGER_ERROR("RhiSceneRenderer") << "Failed to create RHI scene shader modules.";
        return false;
    }

    return true;
}

rhi::PipelineHandle RhiSceneRenderer::GetOrCreatePipeline(uint32_t vertexStride)
{
    if (auto it = m_PipelinesByStride.find(vertexStride); it != m_PipelinesByStride.end())
        return it->second;

    rhi::GraphicsPipelineDesc desc;
    desc.vertexShader = m_VertexShader;
    desc.fragmentShader = m_FragmentShader;
    desc.debugName = "RHI Forward Unlit";
    desc.vertexInput.bindings.push_back(rhi::VertexBindingDesc{0, vertexStride, rhi::VertexInputRate::PerVertex});
    desc.vertexInput.attributes.push_back(rhi::VertexAttributeDesc{0, 0, rhi::VertexFormat::Float3, 0});
    desc.topology = rhi::PrimitiveTopology::TriangleList;
    desc.rasterizer.cullMode = rhi::CullMode::None;
    desc.rasterizer.frontFace = rhi::FrontFace::CounterClockwise;
    desc.depthStencil.depthTestEnable = true;
    desc.depthStencil.depthWriteEnable = true;
    desc.depthStencil.depthCompare = rhi::CompareOp::Less;
    desc.blendAttachments.push_back(rhi::BlendAttachmentDesc{});
    desc.renderTargetLayout.colorFormats.push_back(m_Backend.GetSwapchain().GetBackBufferFormat());
    desc.renderTargetLayout.depthStencilFormat = rhi::Format::Depth32F;
    desc.pushConstantSize = sizeof(PushConstants);

    rhi::PipelineHandle pipeline = m_Backend.GetDevice().CreateGraphicsPipeline(desc);
    if (!pipeline)
    {
        LOGGER_ERROR("RhiSceneRenderer") << "Failed to create RHI scene pipeline for stride " << vertexStride;
        return {};
    }

    m_PipelinesByStride[vertexStride] = pipeline;
    return pipeline;
}

rhi::PipelineHandle RhiSceneRenderer::GetOrCreateSkyboxPipeline()
{
    if (m_SkyboxPipeline)
        return m_SkyboxPipeline;

    rhi::GraphicsPipelineDesc desc;
    desc.vertexShader = m_SkyboxVS;
    desc.fragmentShader = m_SkyboxPS;
    desc.debugName = "RHI Skybox";
    desc.vertexInput.bindings.push_back(rhi::VertexBindingDesc{0, 12, rhi::VertexInputRate::PerVertex});
    desc.vertexInput.attributes.push_back(rhi::VertexAttributeDesc{0, 0, rhi::VertexFormat::Float3, 0});
    desc.topology = rhi::PrimitiveTopology::TriangleList;
    desc.rasterizer.cullMode = rhi::CullMode::None;
    desc.rasterizer.frontFace = rhi::FrontFace::CounterClockwise;
    desc.depthStencil.depthTestEnable = true;
    desc.depthStencil.depthWriteEnable = false;
    desc.depthStencil.depthCompare = rhi::CompareOp::LessOrEqual;
    desc.blendAttachments.push_back(rhi::BlendAttachmentDesc{});
    desc.renderTargetLayout.colorFormats.push_back(m_Backend.GetSwapchain().GetBackBufferFormat());
    desc.renderTargetLayout.depthStencilFormat = rhi::Format::Depth32F;
    desc.pushConstantSize = sizeof(SkyboxPushConstants);

    m_SkyboxPipeline = m_Backend.GetDevice().CreateGraphicsPipeline(desc);
    return m_SkyboxPipeline;
}

rhi::PipelineHandle RhiSceneRenderer::GetOrCreateLitPipeline(uint32_t vertexStride)
{
    if (auto it = m_LitPipelinesByStride.find(vertexStride); it != m_LitPipelinesByStride.end())
        return it->second;

    rhi::GraphicsPipelineDesc desc;
    desc.vertexShader = m_LitVS;
    desc.fragmentShader = m_LitPS;
    desc.debugName = "RHI Lit PBR";
    desc.vertexInput.bindings.push_back(rhi::VertexBindingDesc{0, vertexStride, rhi::VertexInputRate::PerVertex});
    desc.vertexInput.attributes.push_back(rhi::VertexAttributeDesc{0, 0, rhi::VertexFormat::Float3, 0});
    desc.vertexInput.attributes.push_back(rhi::VertexAttributeDesc{1, 0, rhi::VertexFormat::Float3, 12});
    desc.vertexInput.attributes.push_back(rhi::VertexAttributeDesc{2, 0, rhi::VertexFormat::Float2, 24});
    desc.topology = rhi::PrimitiveTopology::TriangleList;
    desc.rasterizer.cullMode = rhi::CullMode::None;
    desc.rasterizer.frontFace = rhi::FrontFace::CounterClockwise;
    desc.depthStencil.depthTestEnable = true;
    desc.depthStencil.depthWriteEnable = true;
    desc.depthStencil.depthCompare = rhi::CompareOp::Less;
    desc.blendAttachments.push_back(rhi::BlendAttachmentDesc{});
    desc.renderTargetLayout.colorFormats.push_back(m_Backend.GetSwapchain().GetBackBufferFormat());
    desc.renderTargetLayout.depthStencilFormat = rhi::Format::Depth32F;
    desc.pushConstantSize = sizeof(PbrPushConstants);

    rhi::PipelineHandle pipeline = m_Backend.GetDevice().CreateGraphicsPipeline(desc);
    if (pipeline)
        m_LitPipelinesByStride[vertexStride] = pipeline;
    return pipeline;
}

rhi::PipelineHandle RhiSceneRenderer::GetOrCreateDecalPipeline(uint32_t vertexStride)
{
    if (auto it = m_DecalPipelinesByStride.find(vertexStride); it != m_DecalPipelinesByStride.end())
        return it->second;

    rhi::GraphicsPipelineDesc desc;
    desc.vertexShader = m_DecalVS;
    desc.fragmentShader = m_DecalPS;
    desc.debugName = "RHI Decal";
    desc.vertexInput.bindings.push_back(rhi::VertexBindingDesc{0, vertexStride, rhi::VertexInputRate::PerVertex});
    desc.vertexInput.attributes.push_back(rhi::VertexAttributeDesc{0, 0, rhi::VertexFormat::Float3, 0});
    desc.vertexInput.attributes.push_back(rhi::VertexAttributeDesc{1, 0, rhi::VertexFormat::Float3, 12});
    desc.vertexInput.attributes.push_back(rhi::VertexAttributeDesc{2, 0, rhi::VertexFormat::Float2, 24});
    desc.topology = rhi::PrimitiveTopology::TriangleList;
    desc.rasterizer.cullMode = rhi::CullMode::None;
    desc.rasterizer.frontFace = rhi::FrontFace::CounterClockwise;
    desc.depthStencil.depthTestEnable = true;
    desc.depthStencil.depthWriteEnable = false;
    desc.depthStencil.depthCompare = rhi::CompareOp::LessOrEqual;

    rhi::BlendAttachmentDesc blend;
    blend.blendEnable = true;
    blend.srcColorBlendFactor = rhi::BlendFactor::SrcAlpha;
    blend.dstColorBlendFactor = rhi::BlendFactor::OneMinusSrcAlpha;
    blend.colorBlendOp = rhi::BlendOp::Add;
    blend.srcAlphaBlendFactor = rhi::BlendFactor::SrcAlpha;
    blend.dstAlphaBlendFactor = rhi::BlendFactor::OneMinusSrcAlpha;
    blend.alphaBlendOp = rhi::BlendOp::Add;
    desc.blendAttachments.push_back(blend);

    desc.renderTargetLayout.colorFormats.push_back(m_Backend.GetSwapchain().GetBackBufferFormat());
    desc.renderTargetLayout.depthStencilFormat = rhi::Format::Depth32F;
    desc.pushConstantSize = sizeof(DecalPushConstants);

    rhi::PipelineHandle pipeline = m_Backend.GetDevice().CreateGraphicsPipeline(desc);
    if (pipeline)
        m_DecalPipelinesByStride[vertexStride] = pipeline;
    return pipeline;
}

void RhiSceneRenderer::DestroyPipelines()
{
    auto& device = m_Backend.GetDevice();
    for (auto& [_, pipeline] : m_PipelinesByStride)
    {
        if (pipeline)
            device.DestroyPipeline(pipeline);
    }
    m_PipelinesByStride.clear();

    for (auto& [_, pipeline] : m_LitPipelinesByStride)
    {
        if (pipeline)
            device.DestroyPipeline(pipeline);
    }
    m_LitPipelinesByStride.clear();

    for (auto& [_, pipeline] : m_DecalPipelinesByStride)
    {
        if (pipeline)
            device.DestroyPipeline(pipeline);
    }
    m_DecalPipelinesByStride.clear();

    if (m_SkyboxPipeline)
    {
        device.DestroyPipeline(m_SkyboxPipeline);
        m_SkyboxPipeline = {};
    }
}

std::vector<uint8_t> RhiSceneRenderer::LoadShaderFile(const std::string& relativePath) const
{
    std::string path = ShaderAssetPath(relativePath);
    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        path = FileSystem::getPath((std::filesystem::path("include/engine/asset/shaders") / relativePath).string());
        file.open(path, std::ios::binary);
    }
    if (!file)
    {
        LOGGER_ERROR("RhiSceneRenderer") << "Missing RHI shader file: " << relativePath;
        return {};
    }

    file.seekg(0, std::ios::end);
    const std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    if (size <= 0)
        return {};

    std::vector<uint8_t> bytes(static_cast<size_t>(size));
    file.read(reinterpret_cast<char*>(bytes.data()), size);
    return bytes;
}
