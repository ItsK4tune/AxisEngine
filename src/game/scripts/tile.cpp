#include <game/scripts/tile.h>
#include <engine/ecs/component.h>
#include <iostream>

void Tile::OnCreate() {
    SetVisibility(TileVisibility::HIDDEN);
}

void Tile::SetVisibility(TileVisibility newStatus) {
    visibility = newStatus;
    
    if (visibility == TileVisibility::VISIBLE) {
        isExplored = true;
    }

    if (HasComponent<MeshRendererComponent>()) {
        auto& render = GetComponent<MeshRendererComponent>();
        
        switch (visibility) {
            case TileVisibility::VISIBLE:
                render.visible = true;
                render.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
                break;
                
            case TileVisibility::FOGGED:
                if (isExplored) {
                    render.visible = true;
                    render.color = glm::vec4(0.3f, 0.3f, 0.5f, 1.0f);
                } else {
                    render.visible = false;
                }
                break;
                
            case TileVisibility::HIDDEN:
                render.visible = false;
                break;
        }
    }
}