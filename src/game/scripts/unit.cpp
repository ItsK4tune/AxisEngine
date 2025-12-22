#include <game/scripts/unit.h>
#include <game/scripts/skill_registry.h>
#include <engine/utils/filesystem.h>
#include <engine/utils/bullet_glm_helpers.h>

void Unit::OnCreate()
{
    if (HasComponent<TransformComponent>())
    {
        auto &trans = GetComponent<TransformComponent>();
        trans.position = HexMath::HexToWorld(gridPos);
        SyncPhysics(trans.position);
    }
}

void Unit::OnUpdate(float dt)
{
    if (isMoving && pathIndex < movePath.size())
    {
        auto &trans = GetComponent<TransformComponent>();
        glm::vec3 target = movePath[pathIndex];
        glm::vec3 dir = target - trans.position;

        if (glm::length2(dir) < 0.01f)
        {
            trans.position = target;
            pathIndex++;

            if (pathIndex >= movePath.size())
                isMoving = false;
        }
        else
        {
            trans.position += glm::normalize(dir) * moveSpeed * dt;
        }

        SyncPhysics(trans.position);
    }
}

void Unit::LoadFromFile(const std::string &path)
{
    std::ifstream file(FileSystem::getPath(path).c_str());
    std::string line, key;

    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;
        std::stringstream ss(line);
        ss >> key;

        if (key == "HP")
            ss >> stats.maxHP;
        else if (key == "PHYS")
            ss >> stats.physicDmg;
        else if (key == "ELE")
            ss >> stats.physicDmg;
        else if (key == "RANGE")
            ss >> stats.physicDmg;
        else if (key == "CRIT_CHANCE")
            ss >> stats.physicDmg;
        else if (key == "DEF")
            ss >> stats.defense;
        else if (key == "RES")
            ss >> stats.resistance;
        else if (key == "EVAS")
            ss >> stats.resistance;
        else if (key == "GUARD")
            ss >> stats.moveCost;
        else if (key == "SYNC")
            ss >> stats.moveCost;
        else if (key == "MOVE_RADIUS")
            ss >> stats.moveCost;
        else if (key == "LIGHT_RADIUS")
            ss >> stats.moveCost;
        else if (key == "MOVE_COST")
            ss >> stats.actionCost;
        else if (key == "ACTION_COST")
            ss >> stats.actionCost;

        else if (key == "MODEL")
        {
            std::string modelName;
            ss >> modelName;

            if(HasComponent<MeshRendererComponent>()) {
                auto& ren = GetComponent<MeshRendererComponent>();
                ren.model = m_App->GetResourceManager().GetModel(modelName);
            }
        }

        else if (key == "SHADER") {
            std::string shaderName;
            ss >> shaderName;
            
            if(HasComponent<MeshRendererComponent>()) {
                auto& ren = GetComponent<MeshRendererComponent>();
                ren.shader = m_App->GetResourceManager().GetShader(shaderName);
            }
        }

        else if (key == "SKILLS")
        {
            std::string skillName;
            while (ss >> skillName)
            {
                if (skillName.back() == '|')
                    skillName.pop_back();
                auto skill = SkillRegistry::CreateSkill(skillName);
                if (skill)
                    skills.push_back(skill);
            }
        }
    }
    stats.currentHP = stats.maxHP;

    std::cout << "[Unit] Loaded unit from " << path << std::endl;
}

void Unit::MoveByPath(const std::vector<HexCoord> &path)
{
    movePath.clear();
    for (auto &h : path)
        movePath.push_back(HexMath::HexToWorld(h));

    pathIndex = 0;
    isMoving = true;
}

void Unit::Attack(Unit *target)
{
    if (!target)
        return;
    stats.isGuarding = false;
    target->TakeDamage(stats.physicDmg, stats.elementalDmg, this);
}

void Unit::Guard()
{
    stats.isGuarding = true;
    std::cout << "[Unit] Guarding.\n";
}

void Unit::TakeDamage(int rawPhys, int rawElem, Unit *attacker)
{
    if ((rand() % 100) < stats.evasion)
    {
        std::cout << "[Unit] MISS!\n";
        return;
    }

    float curDef = (float)stats.defense;
    float curRes = (float)stats.resistance;
    if (stats.isGuarding)
    {
        curDef *= (1.0f + stats.guardBonus / 100.0f);
        curRes *= (1.0f + stats.guardBonus / 100.0f);
    }

    bool isCrit = (rand() % 100) < attacker->stats.critChance;
    if (isCrit)
        std::cout << "[Unit] CRIT!\n";
    float critMult = isCrit ? 1.75f : 1.0f;
    float defMit = 100.0f / (100.0f + curDef);
    float finalPhys = rawPhys * critMult * defMit;

    float resMit = (100.0f - curRes) / 100.0f;
    if (resMit < 0)
        resMit = 0;

    float syncBonus = 1.0f;
    if (stats.isAfflicted)
    {
        syncBonus = (100.0f + attacker->stats.synchronizeBonus) / 100.0f;
        std::cout << "[Unit] SYNCHRONIZE!\n";
        stats.isAfflicted = false;
    }
    else
    {
        if (rawElem > 0)
            stats.isAfflicted = true;
    }
    float finalElem = rawElem * resMit * syncBonus;

    float total = (std::max)(1.0f, finalPhys + finalElem);
    stats.currentHP -= total;

    std::cout << "[Unit] physic: " << finalPhys << " elemental: " << finalElem << " (HP: " << stats.currentHP << ")\n";

    if (stats.currentHP <= 0)
        Die();
}

void Unit::Die()
{
    std::cout << "[Unit] Unit Died!\n";

    if (HasComponent<TransformComponent>())
        GetComponent<TransformComponent>().scale = glm::vec3(0.0f);

    if (HasComponent<RigidBodyComponent>())
    {
        auto &rb = GetComponent<RigidBodyComponent>();
        if (rb.body)
        {
            btTransform t = rb.body->getWorldTransform();
            t.setOrigin(btVector3(0, -999, 0));
            rb.body->setWorldTransform(t);
        }
    }
}

void Unit::ResetState()
{
    stats.isGuarding = false;
}

void Unit::SyncPhysics(const glm::vec3 &pos)
{
    if (HasComponent<RigidBodyComponent>())
    {
        auto &rb = GetComponent<RigidBodyComponent>();
        if (rb.body)
        {
            btTransform t = rb.body->getWorldTransform();
            t.setOrigin(BulletGLMHelpers::convert(pos));
            rb.body->setWorldTransform(t);

            if (rb.body->getMotionState())
            {
                rb.body->getMotionState()->setWorldTransform(t);
            }

            rb.body->activate(true);
        }
    }
}