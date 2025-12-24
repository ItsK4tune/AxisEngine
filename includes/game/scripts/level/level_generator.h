#pragma once

#include <engine/core/application.h>
#include <game/scripts/level/level_model.h>

class LevelGenerator
{
public:
    static void InitTeams(Scene *scene, Application *app, LevelModel &level);
    static void LoadMap(const std::string &path, Scene *scene, Application *app, LevelModel &level);
    static void SpawnUnit(int q, int r, int h, int teamID, Scene *scene, Application *app, LevelModel &level);
    static void CreateHexTile(int q, int r, int h, Scene *scene, Application *app, std::vector<Tile *> &tiles);
};