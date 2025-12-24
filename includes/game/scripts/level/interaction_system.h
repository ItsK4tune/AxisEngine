#pragma once

#include <game/scripts/level/level_model.h>
#include <engine/core/application.h>

class InteractionSystem
{
public:
    static void Update(LevelModel &level, Scene *scene, Application *app, float dt);

private:
    static void HandleClick(LevelModel &level, Scene *scene, Application *app);
    static void UpdateUI(LevelModel &level, Scene *scene);
    static void GetMouseRay(glm::vec3 &outOrigin, glm::vec3 &outEnd,  Scene *scene, Application *app);
};