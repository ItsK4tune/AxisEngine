#include <game/scripts/level/level_generator.h>

#include <engine/utils/filesystem.h>
#include <engine/utils/bullet_glm_helpers.h>
#include <game/scripts/team/team.h>
#include <game/scripts/unit/unit.h>
#include <game/scripts/tile/tile.h>
#include <fstream>
#include <iostream>

void LevelGenerator::InitTeams(Scene *scene, Application *app, LevelModel &level)
{
    auto createTeam = [&](std::string name) -> Team *
    {
        auto e = scene->createEntity();
        scene->registry.emplace<InfoComponent>(e, name, "team");
        auto &sc = scene->registry.emplace<ScriptComponent>(e);
        sc.Bind<Team>();
        sc.instance = sc.InstantiateScript();
        sc.instance->Init(e, scene, app);
        sc.instance->OnCreate();
        auto *t = static_cast<Team *>(sc.instance);
        return t;
    };

    level.team1 = createTeam("Team1");
    level.team2 = createTeam("Team2");
}

void LevelGenerator::LoadMap(const std::string &path, Scene *scene, Application *app, LevelModel &level)
{
    std::ifstream file(FileSystem::getPath(path).c_str());

    if (!file.is_open())
    {
        std::cerr << "[LevelGenerator] Could not open map file: " << FileSystem::getPath(path).c_str() << std::endl;
        return;
    }

    int w, h, d;
    if (!(file >> w >> h >> d))
    {
        std::cerr << "[LevelGenerator] Need width, height and layer: " << FileSystem::getPath(path).c_str() << std::endl;
        return;
    }

    std::string dummy;
    std::getline(file, dummy);

    for (int layer = 0; layer < d; layer++)
    {
        for (int row = 0; row < h; row++)
        {
            for (int col = 0; col < w; col++)
            {
                while (file.peek() == '#' || isspace(file.peek()))
                {
                    if (file.peek() == '#')
                        std::getline(file, dummy);
                    else
                        file.get();
                }
                int hasTile;
                if (file >> hasTile && hasTile)
                {
                    int q = col - (row - (row & 1)) / 2;
                    CreateHexTile(q, row, layer, scene, app, level.tiles);
                }
            }
        }
    }
}

void LevelGenerator::SpawnUnit(int q, int r, int h, Team* team, Scene *scene, Application *app, LevelModel &level)
{
    auto e = scene->createEntity();
    scene->registry.emplace<InfoComponent>(e, "Unit", "unit");

    auto &t = scene->registry.emplace<TransformComponent>(e);
    t.scale = glm::vec3(0.01f);
    scene->registry.emplace<MeshRendererComponent>(e).castShadow = true;

    auto &sc = scene->registry.emplace<ScriptComponent>(e);
    sc.Bind<Unit>();
    sc.instance = sc.InstantiateScript();
    sc.instance->Init(e, scene, app);

    Unit *unit = static_cast<Unit *>(sc.instance);
    unit->state.gridPos = {q, r, h};
    unit->state.team = team;

    if (team == level.team1)
        unit->InitFromFile("resources/units/knight.unit");
    else
        unit->InitFromFile("resources/units/knight.unit");

    sc.instance->OnCreate();

    auto &rb = scene->registry.emplace<RigidBodyComponent>(e);
    btCapsuleShape *shape = new btCapsuleShape(0.5f, 2.0f);
    btTransform trans;
    trans.setIdentity();
    trans.setOrigin(BulletGLMHelpers::convert(t.position));
    rb.body = app->GetPhysicsWorld().CreateRigidBody(10.0f, trans, shape);
    rb.body->setUserIndex((int)e);
    rb.body->setCollisionFlags(rb.body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
    rb.body->setActivationState(DISABLE_DEACTIVATION);

    if (team == level.team1)
        level.team1->AddUnit(e);
    else
        level.team2->AddUnit(e);
}

void LevelGenerator::CreateHexTile(int q, int r, int h, Scene *scene, Application *app, std::vector<Tile *> &tiles)
{
    auto e = scene->createEntity();
    scene->registry.emplace<InfoComponent>(e, "Tile", "tile");

    auto &t = scene->registry.emplace<TransformComponent>(e);
    t.position = HexMath::HexToWorld({q, r, h});
    t.scale = glm::vec3(0.0003f);

    auto &ren = scene->registry.emplace<MeshRendererComponent>(e);
    ren.model = app->GetResourceManager().GetModel("hex_model");
    ren.shader = app->GetResourceManager().GetShader("modelShader");

    auto &rb = scene->registry.emplace<RigidBodyComponent>(e);
    btBoxShape *shape = new btBoxShape(btVector3(0.5f, 0.25f, 0.5f));
    btTransform trans;
    trans.setIdentity();
    trans.setOrigin(BulletGLMHelpers::convert(t.position));
    rb.body = app->GetPhysicsWorld().CreateRigidBody(0.0f, trans, shape);
    rb.body->setUserIndex((int)e);

    auto &sc = scene->registry.emplace<ScriptComponent>(e);
    sc.Bind<Tile>();
    sc.instance = sc.InstantiateScript();
    sc.instance->Init(e, scene, app);
    auto tile = static_cast<Tile *>(sc.instance);
    tile->gridPos = {q, r, h};
    sc.instance->OnCreate();

    tiles.push_back(tile);
}