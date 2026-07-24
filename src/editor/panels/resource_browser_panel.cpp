#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <editor/panels/resource_browser_panel.h>

#ifdef ENABLE_EDITOR
#include <audio/interface/i_audio_engine.h>
#include <audio/interface/i_audio_source.h>
#include <audio/interface/i_sound.h>
#include <core/logic/service_locator.h>
#include <render/logic/video_decoder.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_texture_manager.h>
#include <render/type/graphics_types.h>
#include <render/unit/skybox.h>
#include <resource/logic/resource_manager.h>
#include <resource/type/fragment_asset.h>
#include <resource/unit/animation.h>
#include <resource/unit/font.h>
#include <resource/unit/model.h>
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>
namespace fs = std::filesystem;

namespace
{
void ScanSourceDirectories(const std::vector<fs::path>& directories,
                           std::vector<std::pair<std::string, std::string>>& output)
{
    output.clear();
    std::error_code error;
    for (const fs::path& directory : directories)
    {
        if (!fs::is_directory(directory, error))
        {
            error.clear();
            continue;
        }
        for (fs::directory_iterator iterator(directory, error), end; iterator != end && !error;
             iterator.increment(error))
        {
            if (!iterator->is_regular_file(error))
                continue;
            const std::string extension = iterator->path().extension().string();
            if (extension == ".cpp" || extension == ".h" || extension == ".hpp")
                output.emplace_back(iterator->path().filename().string(), iterator->path().string());
        }
        error.clear();
    }
    std::sort(output.begin(), output.end());
    output.erase(std::unique(output.begin(), output.end()), output.end());
}
}

void ResourceBrowserPanel::Initialize()
{
    RefreshSourceFiles();
}

void ResourceBrowserPanel::RefreshSourceFiles()
{
    ScanSourceDirectories({"src/scripts", "include/scripts"}, m_Scripts);
    ScanSourceDirectories({"src/states", "include/states"}, m_States);
}

static glm::vec3 RotatePoint(const glm::vec3& p, float rotX, float rotY)
{
    float cx = std::cos(rotX);
    float sx = std::sin(rotX);
    glm::vec3 r1 = glm::vec3(p.x, p.y * cx - p.z * sx, p.y * sx + p.z * cx);

    float cy = std::cos(rotY);
    float sy = std::sin(rotY);
    return glm::vec3(r1.x * cy + r1.z * sy, r1.y, -r1.x * sy + r1.z * cy);
}

#undef GetCurrentTime
void ResourceBrowserPanel::OnImGui(Scene& scene)
{
    ImGui::Begin(GetTitle().c_str(), &m_Open);
    DrawContents(scene);
    ImGui::End();
}

void ResourceBrowserPanel::DrawContents(Scene& scene)
{
    auto* rm = ServiceLocator::Instance().Resolve<ResourceManager>();
    if (!rm)
    {
        ImGui::Text("ResourceManager is not available.");
        return;
    }

    float availW = ImGui::GetContentRegionAvail().x;
    float leftW = availW * 0.40f;
    float rightW = availW - leftW - ImGui::GetStyle().ItemSpacing.x;

    ImGui::BeginChild("ResourceListPane", ImVec2(leftW, 0), true);

    if (ImGui::BeginTabBar("ResourceTabs"))
    {
        if (ImGui::BeginTabItem("Meshes"))
        {
            m_ActiveTab = 0;
            auto models = rm->GetLoadedModels();
            if (models.empty())
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No meshes loaded.");
            for (const auto& name : models)
            {
                bool selected = (m_SelectedName == name && m_SelectedType == "Mesh");
                if (ImGui::Selectable(name.c_str(), selected))
                {
                    m_SelectedName = name;
                    m_SelectedType = "Mesh";
                }
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Textures"))
        {
            m_ActiveTab = 1;
            auto textures = rm->GetLoadedTextures();
            if (textures.empty())
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No textures loaded.");
            for (const auto& name : textures)
            {
                bool selected = (m_SelectedName == name && m_SelectedType == "Texture");
                if (ImGui::Selectable(name.c_str(), selected))
                {
                    m_SelectedName = name;
                    m_SelectedType = "Texture";
                }
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Shaders"))
        {
            m_ActiveTab = 2;
            auto shaders = rm->GetLoadedShaders();
            if (shaders.empty())
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No shaders loaded.");
            for (const auto& name : shaders)
            {
                bool selected = (m_SelectedName == name && m_SelectedType == "Shader");
                if (ImGui::Selectable(name.c_str(), selected))
                {
                    m_SelectedName = name;
                    m_SelectedType = "Shader";
                }
            }
            ImGui::SeparatorText("Compute");
            auto computeShaders = rm->GetLoadedComputeShaders();
            if (computeShaders.empty())
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No compute shaders loaded.");
            for (const auto& name : computeShaders)
            {
                bool selected = (m_SelectedName == name && m_SelectedType == "ComputeShader");
                if (ImGui::Selectable(name.c_str(), selected))
                {
                    m_SelectedName = name;
                    m_SelectedType = "ComputeShader";
                }
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Audio"))
        {
            m_ActiveTab = 3;
            auto sounds = rm->GetLoadedSounds();
            if (sounds.empty())
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No audio loaded.");
            for (const auto& name : sounds)
            {
                bool selected = (m_SelectedName == name && m_SelectedType == "Audio");
                if (ImGui::Selectable(name.c_str(), selected))
                {
                    m_SelectedName = name;
                    m_SelectedType = "Audio";
                    if (m_ActiveSound)
                    {
                        m_ActiveSound->Stop();
                        m_ActiveSound.reset();
                    }
                }
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Video"))
        {
            m_ActiveTab = 4;
            auto videos = rm->GetLoadedVideos();
            if (videos.empty())
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No video loaded.");
            for (const auto& name : videos)
            {
                bool selected = (m_SelectedName == name && m_SelectedType == "Video");
                if (ImGui::Selectable(name.c_str(), selected))
                {
                    m_SelectedName = name;
                    m_SelectedType = "Video";
                }
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Animations"))
        {
            m_ActiveTab = 5;
            auto anims = rm->GetLoadedAnimations();
            if (anims.empty())
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No animations loaded.");
            for (const auto& name : anims)
            {
                bool selected = (m_SelectedName == name && m_SelectedType == "Animation");
                if (ImGui::Selectable(name.c_str(), selected))
                {
                    m_SelectedName = name;
                    m_SelectedType = "Animation";
                }
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Skyboxes"))
        {
            m_ActiveTab = 6;
            auto skyboxes = rm->GetLoadedSkyboxes();
            if (skyboxes.empty())
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No skyboxes loaded.");
            for (const auto& name : skyboxes)
            {
                bool selected = (m_SelectedName == name && m_SelectedType == "Skybox");
                if (ImGui::Selectable(name.c_str(), selected))
                {
                    m_SelectedName = name;
                    m_SelectedType = "Skybox";
                }
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Fonts"))
        {
            m_ActiveTab = 7;
            auto fonts = rm->GetLoadedFonts();
            if (fonts.empty())
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No fonts loaded.");
            for (const auto& name : fonts)
            {
                bool selected = (m_SelectedName == name && m_SelectedType == "Font");
                if (ImGui::Selectable(name.c_str(), selected))
                {
                    m_SelectedName = name;
                    m_SelectedType = "Font";
                }
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Fragments"))
        {
            m_ActiveTab = 8;
            auto frags = rm->GetLoadedFragments();
            if (frags.empty())
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No fragments loaded.");
            for (const auto& name : frags)
            {
                bool selected = (m_SelectedName == name && m_SelectedType == "Fragment");
                if (ImGui::Selectable(name.c_str(), selected))
                {
                    m_SelectedName = name;
                    m_SelectedType = "Fragment";
                }
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Scriptable"))
        {
            m_ActiveTab = 9;
            if (ImGui::SmallButton("Refresh source files"))
                RefreshSourceFiles();
            if (m_Scripts.empty())
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                                   "No scripts found in src/scripts or include/scripts.");
            for (const auto& item : m_Scripts)
            {
                bool selected = (m_SelectedName == item.first && m_SelectedType == "Scriptable");
                if (ImGui::Selectable(item.first.c_str(), selected))
                {
                    m_SelectedName = item.first;
                    m_SelectedType = "Scriptable";
                    m_SelectedPath = item.second;
                }
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("State"))
        {
            m_ActiveTab = 10;
            if (ImGui::SmallButton("Refresh source files"))
                RefreshSourceFiles();
            if (m_States.empty())
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No states found in src/states or include/states.");
            for (const auto& item : m_States)
            {
                bool selected = (m_SelectedName == item.first && m_SelectedType == "State");
                if (ImGui::Selectable(item.first.c_str(), selected))
                {
                    m_SelectedName = item.first;
                    m_SelectedType = "State";
                    m_SelectedPath = item.second;
                }
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("ResourceDetailsPane", ImVec2(rightW, 0), true);

    if (m_SelectedName.empty())
    {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Select a resource to view details and preview.");
    }
    else
    {
        ImGui::TextColored(ImVec4(0.0f, 0.8f, 1.0f, 1.0f), "%s: %s", m_SelectedType.c_str(), m_SelectedName.c_str());
        ImGui::Separator();
        ImGui::Spacing();

        if (m_SelectedType == "Mesh")
        {
            auto model = rm->GetModel(m_SelectedName);
            if (model)
            {
                ImGui::Text("Directory: %s", model->directory.c_str());
                ImGui::Text("Is Static: %s", model->IsStatic() ? "Yes" : "No");
                ImGui::Text("Gamma Correction: %s", model->gammaCorrection ? "Enabled" : "Disabled");
                ImGui::Text("Render Ready: %s", model->IsReadyToRender() ? "Yes" : "No");

                size_t vertCount = 0;
                size_t triCount = 0;
                for (const auto& m : model->meshes)
                {
                    vertCount += m.m_VertexCount;
                    triCount += m.indices.size() / 3;
                }
                ImGui::Text("Meshes Count: %d", (int)model->meshes.size());
                ImGui::Text("Total Vertices: %d", (int)vertCount);
                ImGui::Text("Total Triangles: %d", (int)triCount);
                ImGui::Text("AABB Min: (%.2f, %.2f, %.2f)", model->aabb.minBound.x, model->aabb.minBound.y,
                            model->aabb.minBound.z);
                ImGui::Text("AABB Max: (%.2f, %.2f, %.2f)", model->aabb.maxBound.x, model->aabb.maxBound.y,
                            model->aabb.maxBound.z);

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.0f, 1.0f),
                                   "3D Wireframe Preview (Drag to Rotate, Scroll to Zoom):");

                float initialScrollY = ImGui::GetScrollY();
                ImVec2 boxStart = ImGui::GetCursorScreenPos();
                ImVec2 previewSize(256, 256);
                ImGui::InvisibleButton("3DPreviewBox", previewSize);

                if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
                {
                    ImVec2 delta = ImGui::GetIO().MouseDelta;
                    m_RotY += delta.x * 0.01f;
                    m_RotX += delta.y * 0.01f;
                }
                float scroll = ImGui::GetIO().MouseWheel;
                if (ImGui::IsItemHovered())
                {
                    if (scroll != 0.0f)
                    {
                        m_ZoomFactor += scroll * 0.1f;
                        if (m_ZoomFactor < 0.1f)
                            m_ZoomFactor = 0.1f;
                    }
                    ImGui::SetScrollY(initialScrollY);  // Lock parent panel scroll
                }

                ImDrawList* drawList = ImGui::GetWindowDrawList();
                drawList->AddRectFilled(boxStart, ImVec2(boxStart.x + previewSize.x, boxStart.y + previewSize.y),
                                        IM_COL32(20, 20, 25, 255), 8.0f);
                drawList->AddRect(boxStart, ImVec2(boxStart.x + previewSize.x, boxStart.y + previewSize.y),
                                  IM_COL32(60, 60, 70, 255), 8.0f, 0, 1.5f);

                // --- Push Clip Rect to prevent lines spilling outside the box! ---
                drawList->PushClipRect(boxStart, ImVec2(boxStart.x + previewSize.x, boxStart.y + previewSize.y), true);

                ImVec2 boxCenter(boxStart.x + previewSize.x * 0.5f, boxStart.y + previewSize.y * 0.5f);

                float extent = glm::length(model->aabb.maxBound - model->aabb.minBound);
                glm::vec3 center = (model->aabb.maxBound + model->aabb.minBound) * 0.5f;

                // Fallback to compute AABB dynamically from vertices if uninitialized
                if (extent < 0.01f)
                {
                    glm::vec3 minB(1e9f), maxB(-1e9f);
                    bool hasVerts = false;
                    for (const auto& m : model->meshes)
                    {
                        for (size_t vIdx = 0; vIdx < m.m_VertexCount; ++vIdx)
                        {
                            const float* posPtr =
                                reinterpret_cast<const float*>(m.m_VertexData.data() + vIdx * m.m_VertexStride);
                            glm::vec3 pos(posPtr[0], posPtr[1], posPtr[2]);
                            minB = glm::min(minB, pos);
                            maxB = glm::max(maxB, pos);
                            hasVerts = true;
                        }
                    }
                    if (hasVerts)
                    {
                        center = (minB + maxB) * 0.5f;
                        extent = glm::length(maxB - minB);
                    }
                }

                float zoom = (extent > 0.001f ? (1.0f / extent) : 0.1f) * m_ZoomFactor;

                float gridR = extent * 0.5f;
                for (int g = -5; g <= 5; ++g)
                {
                    float val = (g / 5.0f) * gridR;
                    glm::vec3 p1 = center + glm::vec3(val, -center.y, -gridR);
                    glm::vec3 p2 = center + glm::vec3(val, -center.y, gridR);
                    glm::vec3 p3 = center + glm::vec3(-gridR, -center.y, val);
                    glm::vec3 p4 = center + glm::vec3(gridR, -center.y, val);

                    glm::vec3 rp1 = RotatePoint(p1 - center, m_RotX, m_RotY);
                    glm::vec3 rp2 = RotatePoint(p2 - center, m_RotX, m_RotY);
                    glm::vec3 rp3 = RotatePoint(p3 - center, m_RotX, m_RotY);
                    glm::vec3 rp4 = RotatePoint(p4 - center, m_RotX, m_RotY);

                    drawList->AddLine(ImVec2(boxCenter.x + rp1.x * zoom, boxCenter.y - rp1.y * zoom),
                                      ImVec2(boxCenter.x + rp2.x * zoom, boxCenter.y - rp2.y * zoom),
                                      IM_COL32(40, 40, 55, 255));
                    drawList->AddLine(ImVec2(boxCenter.x + rp3.x * zoom, boxCenter.y - rp3.y * zoom),
                                      ImVec2(boxCenter.x + rp4.x * zoom, boxCenter.y - rp4.y * zoom),
                                      IM_COL32(40, 40, 55, 255));
                }

                int lineCount = 0;
                int maxLines = 1500;
                for (const auto& mesh : model->meshes)
                {
                    if (mesh.m_VertexCount == 0 || mesh.indices.empty())
                        continue;
                    size_t totalTriangles = mesh.indices.size() / 3;
                    int step = std::max(1, (int)totalTriangles / (maxLines / 3));
                    for (size_t t = 0; t < totalTriangles && lineCount < maxLines; t += step)
                    {
                        size_t idx0 = mesh.indices[3 * t];
                        size_t idx1 = mesh.indices[3 * t + 1];
                        size_t idx2 = mesh.indices[3 * t + 2];
                        if (idx0 >= mesh.m_VertexCount || idx1 >= mesh.m_VertexCount || idx2 >= mesh.m_VertexCount)
                            continue;

                        auto getPos = [&](size_t idx) -> glm::vec3 { return mesh.GetPosition(idx); };
                        glm::vec3 p0 = getPos(idx0) - center;
                        glm::vec3 p1 = getPos(idx1) - center;
                        glm::vec3 p2 = getPos(idx2) - center;

                        glm::vec3 rp0 = RotatePoint(p0, m_RotX, m_RotY);
                        glm::vec3 rp1 = RotatePoint(p1, m_RotX, m_RotY);
                        glm::vec3 rp2 = RotatePoint(p2, m_RotX, m_RotY);

                        ImVec2 s0(boxCenter.x + rp0.x * zoom, boxCenter.y - rp0.y * zoom);
                        ImVec2 s1(boxCenter.x + rp1.x * zoom, boxCenter.y - rp1.y * zoom);
                        ImVec2 s2(boxCenter.x + rp2.x * zoom, boxCenter.y - rp2.y * zoom);

                        drawList->AddLine(s0, s1, IM_COL32(0, 200, 255, 180), 1.0f);
                        drawList->AddLine(s1, s2, IM_COL32(0, 200, 255, 180), 1.0f);
                        drawList->AddLine(s2, s0, IM_COL32(0, 200, 255, 180), 1.0f);
                        lineCount += 3;
                    }
                }

                drawList->PopClipRect();
            }
        }
        else if (m_SelectedType == "Texture")
        {
            auto texture = rm->GetTexture(m_SelectedName);
            if (texture)
            {
                ImGui::Text("Resolution: %d x %d", texture->width, texture->height);
                ImGui::Text("Components: %d", texture->nrComponents);
                ImGui::Text("Path: %s", texture->path.c_str());
                ImGui::Text("OpenGL Texture ID: %u", texture->id);

                ImGui::Spacing();
                if (texture->id != 0)
                {
                    float aspect = (float)texture->width / (texture->height > 0 ? (float)texture->height : 1.0f);
                    ImVec2 imgSize(256.0f, 256.0f / aspect);
                    if (aspect > 1.0f)
                    {
                        imgSize = ImVec2(256.0f, 256.0f / aspect);
                    }
                    float maxH = 256.0f;
                    if (imgSize.y > maxH)
                    {
                        imgSize.x = (imgSize.x / imgSize.y) * maxH;
                        imgSize.y = maxH;
                    }

                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.0f, 1.0f), "2D Preview:");
                    ImGui::Image((void*)(intptr_t)texture->id, imgSize, ImVec2(0, 0), ImVec2(1, 1), ImVec4(1, 1, 1, 1),
                                 ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
                }
                else
                {
                    ImGui::TextColored(ImVec4(1, 0, 0, 1), "No GPU texture loaded.");
                }
            }
        }
        else if (m_SelectedType == "Shader")
        {
            auto shader = rm->GetShader(m_SelectedName);
            if (shader)
            {
                ImGui::Text("Is Deferred: %s", shader->IsDeferred() ? "Yes" : "No");

                std::string vsPath = "";
                std::string fsPath = "";
                for (const auto& def : rm->GetResourceDefinitions())
                {
                    if (def.type == "Shader" && def.name == m_SelectedName)
                    {
                        if (def.properties.count("Vertex"))
                            vsPath = def.properties.at("Vertex");
                        if (def.properties.count("Fragment"))
                            fsPath = def.properties.at("Fragment");
                        break;
                    }
                }
                ImGui::Text("Vertex Shader Path: %s", vsPath.c_str());
                ImGui::Text("Fragment Shader Path: %s", fsPath.c_str());

                if (!fsPath.empty())
                {
                    if (m_CachedShaderPath != fsPath)
                    {
                        m_CachedShaderPath = fsPath;
                        std::ifstream file(fsPath);
                        if (file.is_open())
                        {
                            std::stringstream ss;
                            ss << file.rdbuf();
                            m_CachedShaderCode = ss.str();
                        }
                        else
                        {
                            m_CachedShaderCode = "// Failed to load shader source file.";
                        }
                    }

                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.0f, 1.0f), "Fragment Shader Source Code:");
                    ImGui::InputTextMultiline("##ShaderCode", const_cast<char*>(m_CachedShaderCode.c_str()),
                                              m_CachedShaderCode.size() + 1, ImVec2(0, 300),
                                              ImGuiInputTextFlags_ReadOnly);
                }
            }
        }
        else if (m_SelectedType == "ComputeShader")
        {
            ImGui::Text("Compute shader: %s", m_SelectedName.c_str());
            const auto shader = rm->GetComputeShader(m_SelectedName);
            ImGui::Text("Status: %s", shader && shader->IsValid() ? "Ready" : "Invalid");
        }
        else if (m_SelectedType == "Audio")
        {
            auto soundSource = rm->GetSound(m_SelectedName);
            if (soundSource)
            {
                ImGui::Text("Default Volume: %.2f", soundSource->GetDefaultVolume());
                if (soundSource->SupportsDefaultPitch())
                    ImGui::Text("Default Pitch: %.2f", soundSource->GetDefaultPitch());
                else
                    ImGui::TextDisabled("Default Pitch: unsupported");
                if (soundSource->SupportsDefaultPan())
                    ImGui::Text("Default Pan: %.2f", soundSource->GetDefaultPan());
                else
                    ImGui::TextDisabled("Default Pan: unsupported");
                ImGui::Text("Default Speed: %.2f", soundSource->GetDefaultSpeed());

                ImGui::Spacing();
                auto* ae = ServiceLocator::Instance().Resolve<IAudioEngine>();
                if (ae)
                {
                    if (ImGui::Button("Play 2D Sample"))
                    {
                        m_ActiveSound = ae->Play2D(soundSource.get(), false, false);
                    }
                    if (m_ActiveSound)
                    {
                        ImGui::SameLine();
                        if (ImGui::Button("Stop Sample"))
                        {
                            m_ActiveSound->Stop();
                            m_ActiveSound.reset();
                        }
                    }

                    if (m_ActiveSound && !m_ActiveSound->IsFinished())
                    {
                        ImGui::Spacing();
                        float vol = m_ActiveSound->GetVolume();
                        if (ImGui::SliderFloat("Preview Volume", &vol, 0.0f, 100.0f))
                        {
                            m_ActiveSound->SetVolume(vol);
                        }

                        unsigned int playLen = m_ActiveSound->GetPlayLength();
                        if (playLen > 0)
                        {
                            float progress = (float)m_ActiveSound->GetPlayPosition() / (float)playLen;
                            ImGui::ProgressBar(progress, ImVec2(0, 0), "Playing...");
                        }
                    }
                }
                else
                {
                    ImGui::TextColored(ImVec4(1, 0, 0, 1), "AudioEngine is not available.");
                }
            }
        }
        else if (m_SelectedType == "Video")
        {
            auto video = rm->GetVideo(m_SelectedName);
            if (video)
            {
                video->Update(ImGui::GetIO().DeltaTime);

                ImGui::Text("Resolution: %d x %d", video->GetWidth(), video->GetHeight());
                ImGui::Text("Duration: %.2f seconds", video->GetDuration());
                ImGui::Text("Playback Time: %.2f seconds", video->GetCurrentTime());

                ImGui::Spacing();
                if (ImGui::Button("Play"))
                    video->Play();
                ImGui::SameLine();
                if (ImGui::Button("Pause"))
                    video->Pause();
                ImGui::SameLine();
                if (ImGui::Button("Stop"))
                    video->Stop();

                bool loop = video->IsLooping();
                if (ImGui::Checkbox("Looping", &loop))
                    video->SetLoop(loop);

                if (video->GetTextureID() != 0)
                {
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.0f, 1.0f), "Video Feed (Live):");

                    float aspect =
                        (float)video->GetWidth() / (video->GetHeight() > 0 ? (float)video->GetHeight() : 1.0f);
                    ImVec2 vidSize(256.0f, 256.0f / aspect);
                    ImGui::Image((void*)(intptr_t)video->GetTextureID(), vidSize, ImVec2(0, 0), ImVec2(1, 1),
                                 ImVec4(1, 1, 1, 1), ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
                }
            }
            else
            {
                ImGui::Text("No active video feed found.");
            }
        }
        else if (m_SelectedType == "Animation")
        {
            auto anim = rm->GetAnimation(m_SelectedName);
            if (anim)
            {
                ImGui::Text("Duration: %.2f ticks", anim->GetDuration());
                ImGui::Text("Ticks Per Second: %.2f", anim->GetTicksPerSecond());
                ImGui::Text("Bones Animated: %d", (int)anim->GetBoneIDMap().size());
            }
        }
        else if (m_SelectedType == "Skybox")
        {
            auto skybox = rm->GetSkybox(m_SelectedName);
            if (skybox)
            {
                ImGui::Text("Skybox loaded successfully.");
                if (skybox->GetTextureID() != 0)
                {
                    ImGui::Text("OpenGL Texture ID: %u", skybox->GetTextureID());
                }
            }
        }
        else if (m_SelectedType == "Font")
        {
            auto font = rm->GetFont(m_SelectedName);
            if (font)
            {
                ImGui::Text("Font name: %s", m_SelectedName.c_str());
                ImGui::Text("Font Size: %u px", font->GetFontSize());

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.0f, 1.0f), "Interactive Font Previewer:");
                static char fontTestText[128] = "Axis Engine 123";
                ImGui::InputText("Test Typing", fontTestText, sizeof(fontTestText));

                ImGui::Spacing();
                ImGui::BeginChild("FontGlyphsBox", ImVec2(0, 110), true, ImGuiWindowFlags_HorizontalScrollbar);

                float baselineY = 75.0f;  // Typrographic baseline from top of child window
                ImVec2 startCursor = ImGui::GetCursorPos();
                float currentX = startCursor.x;

                for (int i = 0; fontTestText[i] != '\0'; ++i)
                {
                    char c = fontTestText[i];
                    if (c == ' ')
                    {
                        currentX += 14.0f;  // Advance space
                        continue;
                    }
                    const Character& ch = font->GetCharacter(static_cast<unsigned int>(c));
                    if (ch.textureID != 0)
                    {
                        ImVec2 glyphSize(static_cast<float>(ch.size.x), static_cast<float>(ch.size.y));
                        if (glyphSize.x > 0 && glyphSize.y > 0)
                        {
                            // Align glyph top vertically relative to typographic baseline
                            float topY = startCursor.y + baselineY - ch.bearing.y;

                            ImGui::SetCursorPos(ImVec2(currentX, topY));

                            if (auto* graphics = ServiceLocator::Instance().Resolve<IGraphicsContext>())
                                graphics->GetTextureManager().SetTextureSwizzle(
                                    TextureType::Texture2D, ch.textureID, TextureSwizzle::RedToRGBA);

                            // Render with standard OpenGL texture coordinates & custom cyan/blue tinting
                            ImGui::Image((void*)(intptr_t)ch.textureID, glyphSize,
                                         ImVec2(ch.uvMin.x, ch.uvMin.y), ImVec2(ch.uvMax.x, ch.uvMax.y),
                                         ImVec4(0.0f, 0.8f, 1.0f, 1.0f), ImVec4(0.1f, 0.1f, 0.15f, 1.0f));

                            // Advance X cursor
                            currentX += glyphSize.x + 2.0f;
                        }
                    }
                }
                ImGui::EndChild();
            }
        }
        else if (m_SelectedType == "Fragment")
        {
            auto frag = rm->GetFragment(m_SelectedName);
            if (frag)
            {
                ImGui::Text("Fragment Path: %s", frag->path.c_str());
            }
        }
        else if (m_SelectedType == "Scriptable" || m_SelectedType == "State")
        {
            ImGui::Text("File Path: %s", m_SelectedPath.c_str());

            if (!m_SelectedPath.empty())
            {
                if (m_CachedShaderPath != m_SelectedPath)
                {
                    m_CachedShaderPath = m_SelectedPath;
                    std::ifstream file(m_SelectedPath);
                    if (file.is_open())
                    {
                        std::stringstream ss;
                        ss << file.rdbuf();
                        m_CachedShaderCode = ss.str();
                    }
                    else
                    {
                        m_CachedShaderCode = "// Failed to load file content.";
                    }
                }

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.0f, 1.0f), "Source Code Preview:");
                ImGui::InputTextMultiline("##SourcePreview", const_cast<char*>(m_CachedShaderCode.c_str()),
                                          m_CachedShaderCode.size() + 1, ImVec2(0, 450), ImGuiInputTextFlags_ReadOnly);
            }
        }
    }
    ImGui::EndChild();

}
#endif
