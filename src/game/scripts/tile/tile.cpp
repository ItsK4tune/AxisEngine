#include <game/scripts/tile/tile.h>
#include <engine/ecs/component.h>
#include <iostream>

void Tile::OnCreate() {
    SetVisibility(TileVisibility::HIDDEN);
}

void Tile::SetVisibility(TileVisibility newStatus)
{
    if (visibility == newStatus)
        return;

    visibility = newStatus;

    if (visibility == TileVisibility::VISIBLE)
        isExplored = true;

    ApplyVisibilityVisual();
}

void Tile::ApplyVisibilityVisual()
{
    if (!HasComponent<MeshRendererComponent>())
        return;

    auto& render = GetComponent<MeshRendererComponent>();

    switch (visibility)
    {
        case TileVisibility::VISIBLE:
            render.visible = true;
            render.color = glm::vec4(1.0f);
            break;

        case TileVisibility::FOGGED:
            if (isExplored)
            {
                render.visible = true;
                render.color = glm::vec4(0.3f, 0.3f, 0.5f, 1.0f);
            }
            else
            {
                render.visible = false;
            }
            break;

        case TileVisibility::HIDDEN:
            render.visible = false;
            break;
    }
}
