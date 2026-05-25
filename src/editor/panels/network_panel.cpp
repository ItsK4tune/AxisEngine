#include <editor/panels/network_panel.h>
#ifdef ENABLE_EDITOR
#include <core/logic/service_locator.h>
#include <ecs/logic/system_manager.h>
#include <engine/network/network_system.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/network_components.h>
#include <editor/panels/scene_hierarchy_panel.h>
#include <scene/logic/scene.h>
#include <imgui.h>

void NetworkPanel::OnImGui(Scene& scene)
{
    ImGui::Begin(GetTitle().c_str(), &m_Open);

    auto* sysMgr = ServiceLocator::Instance().Resolve<SystemManager>();
    if (!sysMgr)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "SystemManager offline");
        ImGui::End();
        return;
    }

    auto* netSys = sysMgr->GetSystem<NetworkSystem>();
    if (!netSys)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "NetworkSystem not registered");
        ImGui::End();
        return;
    }

    static char ip_address[128] = "127.0.0.1";
    static int port = 12345;
    static int max_clients = 32;

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
                    ImGui::Text("Port: %d", port);
                }
                else if (netSys->IsClient())
                {
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Status: Client Connected");
                    ImGui::Text("Connected to: %s:%d", ip_address, port);
                }

                ENetHost* host = netSys->GetHost();
                if (host)
                {
                    ImGui::Separator();
                    ImGui::Text("Performance Metrics:");
                    ImGui::Text("- Connected Peers: %zu", host->connectedPeers);
                    ImGui::Text("- Total Sent: %.2f KB (%u packets)", (float)host->totalSentData / 1024.0f,
                                host->totalSentPackets);
                    ImGui::Text("- Total Received: %.2f KB (%u packets)", (float)host->totalReceivedData / 1024.0f,
                                host->totalReceivedPackets);
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

                ImGui::InputText("IP Address", ip_address, sizeof(ip_address));
                ImGui::InputInt("Port", &port);
                ImGui::InputInt("Max Clients", &max_clients);

                ImGui::Separator();
                if (ImGui::Button("Start Server", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f - 4.0f, 0)))
                {
                    NetworkConfig config;
                    config.host = "";
                    config.port = static_cast<uint16_t>(port);
                    config.maxClients = static_cast<size_t>(max_clients);
                    netSys->StartServer(config);
                }
                ImGui::SameLine();
                if (ImGui::Button("Connect as Client", ImVec2(-1, 0)))
                {
                    NetworkConfig config;
                    config.host = ip_address;
                    config.port = static_cast<uint16_t>(port);
                    netSys->StartClient(config);
                }
            }
            ImGui::EndTabItem();
        }

        // TAB 2: PEERS LIST (Server Only)
        if (ImGui::BeginTabItem("Connected Peers"))
        {
            ENetHost* host = netSys->GetHost();
            if (host && netSys->IsServer())
            {
                ImGui::Text("Connected Clients (%zu/%zu):", host->connectedPeers, host->peerCount);
                if (ImGui::BeginTable("PeersTable", 6,
                                      ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
                {
                    ImGui::TableSetupColumn("ID");
                    ImGui::TableSetupColumn("Address");
                    ImGui::TableSetupColumn("Ping (RTT)");
                    ImGui::TableSetupColumn("Loss");
                    ImGui::TableSetupColumn("Sent / Recv (Data)");
                    ImGui::TableSetupColumn("Actions");
                    ImGui::TableHeadersRow();

                    for (size_t i = 0; i < host->peerCount; ++i)
                    {
                        ENetPeer* peer = &host->peers[i];
                        if (peer->state != ENET_PEER_STATE_CONNECTED)
                            continue;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("%zu", i);

                        ImGui::TableSetColumnIndex(1);
                        char peerIp[64] = "Unknown";
                        enet_address_get_host_ip(&peer->address, peerIp, sizeof(peerIp));
                        ImGui::Text("%s:%d", peerIp, peer->address.port);

                        ImGui::TableSetColumnIndex(2);
                        ImGui::Text("%d ms", peer->roundTripTime);

                        ImGui::TableSetColumnIndex(3);
                        ImGui::Text("%.1f%%", (float)peer->packetLoss * 100.0f / 65536.0f);

                        ImGui::TableSetColumnIndex(4);
                        ImGui::Text("%.1f KB / %.1f KB", (float)peer->totalDataSent / 1024.0f,
                                    (float)peer->totalDataReceived / 1024.0f);

                        ImGui::TableSetColumnIndex(5);
                        ImGui::PushID(static_cast<int>(i));
                        if (ImGui::Button("Kick"))
                        {
                            enet_peer_disconnect(peer, 0);
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

                auto view = scene.registry.view<NetworkComponent>();
                for (auto entity : view)
                {
                    auto& netComp = view.get<NetworkComponent>(entity);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);

                    bool isSelected = (SceneHierarchyPanel::s_SelectedEntity == entity);
                    char label[32];
                    sprintf_s(label, "%u", netComp.networkId);
                    if (ImGui::Selectable(label, isSelected, ImGuiSelectableFlags_SpanAllColumns))
                    {
                        SceneHierarchyPanel::SetSelectedEntity(entity);
                    }

                    ImGui::TableSetColumnIndex(1);
                    if (scene.registry.all_of<InfoComponent>(entity))
                    {
                        auto& info = scene.registry.get<InfoComponent>(entity);
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
                    if (scene.registry.all_of<PositionComponent>(entity))
                    {
                        auto& pos = scene.registry.get<PositionComponent>(entity);
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
