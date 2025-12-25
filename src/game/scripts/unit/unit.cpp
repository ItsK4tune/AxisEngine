#include <game/scripts/unit/unit.h>
#include <engine/core/application.h>
#include <game/commons/utils/hex_math.h>
#include <game/scripts/team/team.h>
#include <game/scripts/unit/unit_loader.h>

void Unit::OnCreate()
{
    if (HasComponent<TransformComponent>())
    {
        auto &trans = GetComponent<TransformComponent>();
        trans.position = HexMath::HexToWorld(state.gridPos);
    }
}

void Unit::OnUpdate(float dt)
{
    if (movement.isMoving)
    {
        auto &trans = GetComponent<TransformComponent>();
        RigidBodyComponent *rb = nullptr;
        if (HasComponent<RigidBodyComponent>())
            rb = &GetComponent<RigidBodyComponent>();

        movement.Update(dt, trans, rb);
    }
}

Team *Unit::GetTeam() const { return state.team; }

std::pair<bool, bool> Unit::CanConsumeAP(int cost) const
{
    return {state.team && state.team->CheckCanConsume(cost).first, state.team && state.team->CheckCanConsume(cost).second};
}

bool Unit::ConsumeMP(int cost)
{
    return state.team && state.team->ConsumeMovePoints(cost);
}

bool Unit::ConsumeAP(int cost)
{
    return state.team && state.team->ConsumeActionPoints(cost);
}

void Unit::InitFromFile(const std::string &path)
{
    MeshRendererComponent *renderer = nullptr;
    if (HasComponent<MeshRendererComponent>())
        renderer = &GetComponent<MeshRendererComponent>();

    UnitLoader::Load(path, stats, skills, m_App->GetResourceManager(), renderer);
}

void Unit::MoveByHexPath(const std::vector<HexCoord> &path)
{
    movement.StartHexPath(path);
    state.gridPos = path.back();
    state.isGuarding = false;
}

void Unit::MoveByWorldPath(const std::vector<glm::vec3> &path)
{
    movement.StartWorldPath(path);
    state.gridPos = HexMath::WorldToHex(path.back());
    state.isGuarding = false;
}

void Unit::Attack(Unit *target)
{
    if (!target)
        return;
    state.isGuarding = false;

    target->ReceiveDamage(stats);
}

void Unit::Guard()
{
    state.isGuarding = true;
    std::cout << "[Unit] Guarding.\n";
}

void Unit::ResetState()
{
    state.isGuarding = false;
}

void Unit::ReceiveDamage(const UnitStats &attackerStats)
{
    DamageResult res = combat.CalculateDamage(attackerStats, stats, state);

    if (res.isMiss)
    {
        std::cout << "[Combat] Missed!\n";
        return;
    }

    if (res.isSync)
    {
        std::cout << "[Combat] Synchronize Boom!\n";
        state.isAfflicted = false;
    }
    else if (attackerStats.elementalDmg > 0)
    {
        state.isAfflicted = true;
    }

    stats.currentHP -= res.finalDamage;
    std::cout << "[Combat] Took " << res.finalPhys << " physical dmg and " << res.finalElem << " elemental dmg. HP: " << stats.currentHP << "\n";

    if (stats.currentHP <= 0)
        Die();
}

void Unit::UsePassiveSkills(SkillTrigger trigger, Unit *target)
{
    for (const auto &skill : skills)
    {
        if (skill->GetType() == SkillType::PASSIVE && skill->CanTrigger(trigger))
        {
            skill->OnTrigger(trigger, this, target);
        }
    }
}

void Unit::UseActiveSkills(SkillTrigger trigger, Unit *target)
{
    for (const auto &skill : skills)
    {
        if (skill->GetType() == SkillType::ACTIVE && skill->CanTrigger(trigger))
        {
            std::cout << "[Unit] Using active skill: " << skill->GetName() << "\n";
            skill->OnTrigger(trigger, this, target);
        }
    }
}

void Unit::Die()
{
    state.isDead = true;
    std::cout << "[Unit] Died.\n";
    if (HasComponent<TransformComponent>())
        GetComponent<TransformComponent>().scale = glm::vec3(0);
}