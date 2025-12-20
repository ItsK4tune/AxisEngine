#pragma once

#include <engine/core/scriptable.h>
#include <engine/core/application.h>
#include <game/commons/utils/hex_math.h>

enum class TileVisibility {
    HIDDEN,
    FOGGED,
    VISIBLE
};

class Tile : public Scriptable {
public:
    HexCoord gridPos {0, 0, 0};
    bool isWalkable = true;
    bool isOccupied = false;
    
    TileVisibility visibility = TileVisibility::HIDDEN;
    bool isExplored = false;

    void OnCreate() override;
    
    void SetVisibility(TileVisibility newStatus);
};