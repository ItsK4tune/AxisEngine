#include <editor/panels/network_panel.h>
#ifdef ENABLE_EDITOR
#include <core/logic/service_locator.h>
#include <network/interface/i_network_service.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/network_components.h>
#include <editor/editor_selection.h>
#include <scene/logic/scene.h>
#include <imgui.h>
#include <algorithm>
#include <cstdio>

void NetworkPanel::OnImGui(Scene& scene)
{
    ImGui::Begin(GetTitle().c_str(), &m_Open);

    auto* netSys = ServiceLocator::Instance().Resolve<INetworkService>();
    if (!netSys)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "NetworkSystem not registered");
        ImGui::End();
        return;
    }

    if (ImGui::BeginTabBar("NetworkPanelTabs"))
    {
        // TAB 1: CONNECTION & CONTROL
        if (ImGui::BeginTabItem("Connection"))
        {
            if (netSys->IsRunning())
            {
                if (netSys->IsServer())
                {
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Status: Server Running");
                    ImGui::Text("Port: %d", m_Port);
                }
                else if (netSys->IsClient())
                {
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Status: Client Connected");
                    ImGui::Text("Connected to: %s:%d", m_IpAddress, m_Port);
                }

                const NetworkStats stats = netSys->GetStats();
                if (netSys->IsRunning())
                {
                    ImGui::Separator();
                    ImGui::Text("Performance Metrics:");
                    ImGui::Text("- Connected Peers: %zu", stats.connectedPeers);
                    ImGui::Text("- Total Sent: %.2f KB (%u packets)", (float)stats.totalSentBytes / 1024.0f,
                                stats.totalSentPackets);
                    ImGui::Text("- Total Received: %.2f KB (%u packets)",
                                (float)stats.totalReceivedBytes / 1024.0f, stats.totalReceivedPackets);
                }

                ImGui::Separator();
                if (ImGui::Button("Stop Network", ImVec2(-1, 0)))
                {
                    netSys->Stop();
                }
            }
            else
            {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Status: Disconnected");
                ImGui::Separator();

                ImGui::InputText("IP Address", m_IpAddress, sizeof(m_IpAddress));
                ImGui::InputInt("Port", &m_Port);
                ImGui::InputInt("Max Clients", &m_MaxClients);
                m_Port = std::clamp(m_Port, 1, 65535);
                m_MaxClients = std::clamp(m_MaxClients, 1, 4096);

                ImGui::Separator();
                if (ImGui::Button("Start Server", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f - 4.0f, 0)))
                {
                    NetworkConfig config;
                    config.host = "";
                    config.port = static_cast<uint16_t>(m_Port);
                    config.maxClients = static_cast<size_t>(m_MaxClients);
                    netSys->StartServer(config);
                }
                ImGui::SameLine();
                if (ImGui::Button("Connect as Client", ImVec2(-1, 0)))
                {
                    NetworkConfig config;
                    config.host = m_IpAddress;
                    config.port = static_cast<uint16_t>(m_Port);
                    netSys->StartClient(config);
                }
            }
            ImGui::EndTabItem();
        }

        // TAB 2: PEERS LIST (Server Only)
        if (ImGui::BeginTabItem("Connected Peers"))
        {
            const auto peers = netSys->GetPeers();
            const auto stats = netSys->GetStats();
            if (netSys->IsRunning() && netSys->IsServer())
            {
                ImGui::Text("Connected Clients (%zu/%zu):", stats.connectedPeers, stats.peerCapacity);
                if (ImGui::BeginTable("PeersTable", 6,
                                      ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
                {
                    ImGui::TableSetupColumn("ID");
                    ImGui::TableSetupColumn("Address");
                    ImGui::TableSetupColumn("Ping (RTT)");
                    ImGui::TableSetupColumn("Loss");
                    ImGui::TableSetupColumn("Throttle");
                    ImGui::TableSetupColumn("Actions");
                    ImGui::TableHeadersRow();

                    for (size_t i = 0; i < peers.size(); ++i)
                    {
                        const auto& peer = peers[i];

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("%zu", i);

                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%s:%d", peer.address.c_str(), peer.port);

                        ImGui::TableSetColumnIndex(2);
                        ImGui::Text("%u ms", peer.roundTripTimeMs);

                        ImGui::TableSetColumnIndex(3);
                        ImGui::Text("%.1f%%", peer.packetLossPercent);

                        ImGui::TableSetColumnIndex(4);
                        ImGui::Text("%u", peer.throttle);

                        ImGui::TableSetColumnIndex(5);
                        ImGui::PushID(static_cast<int>(i));
                        if (ImGui::Button("Kick"))
                        {
                            netSys->DisconnectPeer(peer.id);
                        }
                        ImGui::PopID();
                    }
                    ImGui::EndTable();
                }
            }
            else
            {
                ImGui::Text("Peers list is only available when running as a server.");
            }
            ImGui::EndTabItem();
        }

        // TAB 3: NETWORK REPLICATION (ECS Entities View)
        if (ImGui::BeginTabItem("Replicated Entities"))
        {
            ImGui::Text("Entities containing NetworkComponent:");
            if (ImGui::BeginTable("NetEntitiesTable", 5,
                                  ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
            {
                ImGui::TableSetupColumn("Net ID");
                ImGui::TableSetupColumn("Entity Name");
                ImGui::TableSetupColumn("Owner ID");
                ImGui::TableSetupColumn("Local?");
                ImGui::TableSetupColumn("Position");
                ImGui::TableHeadersRow();

                auto view = scene.View<NetworkComponent>();
                for (auto entity : view)
                {
                    auto& netComp = view.get<NetworkComponent>(entity);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);

                    bool isSelected = ServiceLocator::Instance().Require<EditorSelection>().Contains(entity);
                    char label[32];
                    std::snprintf(label, sizeof(label), "%u", netComp.networkId);
                    if (ImGui::Selectable(label, isSelected, ImGuiSelectableFlags_SpanAllColumns))
                    {
                        ServiceLocator::Instance().Require<EditorSelection>().Select(scene, entity);
                    }

                    ImGui::TableSetColumnIndex(1);
                    if (scene.HasAllComponents<InfoComponent>(entity))
                    {
                        auto& info = scene.GetComponent<InfoComponent>(entity);
                        ImGui::Text("%s", info.name.c_str());
                    }
                    else
                    {
                        ImGui::Text("Entity %d", static_cast<int>(entity));
                    }

                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%u", netComp.ownerId);

                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text(netComp.isLocal ? "Yes" : "No");

                    ImGui::TableSetColumnIndex(4);
                    if (scene.HasAllComponents<PositionComponent>(entity))
                    {
                        auto& pos = scene.GetComponent<PositionComponent>(entity);
                        ImGui::Text("(%.2f, %.2f, %.2f)", pos.value.x, pos.value.y, pos.value.z);
                    }
                    else
                    {
                        ImGui::Text("N/A");
                    }
                }
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}
#endif
