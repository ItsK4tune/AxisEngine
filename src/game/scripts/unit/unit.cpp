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
    return {state.team && state.team->CanConsume(cost).first, state.team && state.team->CanConsume(cost).second};
}

bool Unit::ConsumeMP(int cost)
{
    return state.team && state.team->ConsumeMP(cost);
}

bool Unit::ConsumeAP(int cost)
{
    return state.team && state.team->ConsumeAP(cost);
}

void Unit::InitFromFile(const std::string &path)
{
    MeshRendererComponent *renderer = nullptr;
    if (HasComponent<MeshRendererComponent>())
        renderer = &GetComponent<MeshRendererComponent>();

    UnitLoader::Load(path, stats, skills, m_App->GetResourceManager(), renderer);
}

bool Unit::CanMove(const HexCoord &begin, const HexCoord &end)
{
    std::cout << "[Unit] Checking move from (" << begin.q << "," << begin.r << "," << begin.h << ") to ("
              << end.q << "," << end.r << "," << end.h << ")\n";
    float dist = HexMath::Distance(begin, end);
    std::cout << "[Unit] Distance: " << dist << ", Move Radius: " << stats.moveRadius << "\n";
    return dist <= stats.moveRadius;
}

bool Unit::CanAttack(Unit *target)
{
    if (!target)
        return false;

    int dist = HexMath::Distance(state.gridPos, target->state.gridPos);
    return dist <= stats.attackRange;
}

bool Unit::MoveByWorldPath(const std::vector<glm::vec3> &path)
{
    Team *team = GetTeam();
    if (!team)
    {
        std::cout << "[Unit] Cannot move: No team assigned.\n";
        return false;
    }

    if (!CanMove(state.gridPos, HexMath::WorldToHex(path.back())))
    {
        std::cout << "[Unit] Cannot move: Target too far.\n";
        return false;
    }

    if (!team->CanConsume(stats.moveCost).first)
    {
        std::cout << "[Unit] Cannot move: Not enough MP.\n";
        return false;
    }

    movement.StartWorldPath(path);
    state.gridPos = HexMath::WorldToHex(path.back());
    state.isGuarding = false;

    team->ConsumeMP(stats.moveCost);
    return true;
}

bool Unit::Attack(Unit *target)
{
    Team *team = GetTeam();
    if (!team)
    {
        std::cout << "[Unit] Cannot attack: No team assigned.\n";
        return false;
    }

    if (!CanAttack(target))
    {
        std::cout << "[Unit] Cannot attack: Target too far.\n";
        return false;
    }

    if (!team->CanConsume(stats.actionCost).second)
    {
        std::cout << "[Unit] Cannot attack: Not enough AP.\n";
        return false;
    }

    if (!target)
        return false;
    state.isGuarding = false;

    target->ReceiveDamage(stats);
    team->ConsumeAP(stats.actionCost);
    return true;
}

bool Unit::Guard()
{
    Team *team = GetTeam();
    if (!team)
    {
        std::cout << "[Unit] Cannot attack: No team assigned.\n";
        return false;
    }

    if (!team->CanConsume(stats.actionCost).second)
    {
        std::cout << "[Unit] Cannot attack: Not enough AP.\n";
        return false;
    }

    state.isGuarding = true;
    std::cout << "[Unit] Guarding.\n";
    team->ConsumeAP(stats.actionCost);
    return true;
}

void Unit::OnCycleReset()
{
    state.wantToUseSkill = false;
    state.isGuarding = false;
    state.isAfflicted = false;
}

void Unit::OnPhaseReset()
{
}

void Unit::OnTurnReset()
{
    for (const auto &skill : skills)
    {
        skill->ReduceCooldown();
    }
}

void Unit::ReceiveDamage(const UnitStats &attackerStats)
{
    DamageResult res = combat.CalculateDamage(attackerStats, stats, state);

    if (res.isMiss)
    {
        std::cout << "[Unit] Missed!\n";
        return;
    }

    if (state.isGuarding)
    {
        res.finalDamage = res.finalDamage * stats.guardBonus / 100;
        std::cout << "[Unit] Guarding! Damage reduced to " << res.finalDamage << "\n";
    }

    if (res.isCrit)
    {
        std::cout << "[Unit] Critical Hit!\n";
    }

    if (res.isSync)
    {
        std::cout << "[Unit] Synchronize Boom!\n";
        state.isAfflicted = false;
    }
    else if (attackerStats.elementalDmg > 0)
    {
        state.isAfflicted = true;
    }

    stats.currentHP -= res.finalDamage;
    std::cout << "[Unit] Took " << res.finalPhys << " physical dmg and " << res.finalElem << " elemental dmg. HP: " << stats.currentHP << "\n";

    if (stats.currentHP <= 0)
        Die();
}

bool Unit::UsePassiveSkills(SkillTrigger trigger, Unit *target)
{
    Team *team = GetTeam();
    if (!team)
    {
        std::cout << "[Unit] Cannot use active skill: No team assigned.\n";
        return false;
    }

    bool triggered = false;
    for (const auto &skill : skills)
    {
        if (skill->GetType() == SkillType::PASSIVE && skill->CanTrigger(trigger, this, target))
        {
            std::cout << "[Unit] Using passive skill: " << skill->GetName() << "\n";
            skill->OnTrigger(trigger, this, target);
            triggered = true;
        }
    }

    return triggered;
}

bool Unit::UseActiveSkills(SkillTrigger trigger, Unit *target)
{
    Team *team = GetTeam();
    if (!team)
    {
        std::cout << "[Unit] Cannot use active skill: No team assigned.\n";
        return false;
    }

    bool triggered = false;
    for (const auto &skill : skills)
    {

        if (skill->GetType() == SkillType::ACTIVE && skill->CanTrigger(trigger, this, target))
        {
            std::cout << "[Unit] Using active skill: " << skill->GetName() << "\n";
            skill->OnTrigger(trigger, this, target);
            triggered = true;
        }
    }

    return triggered;
}

void Unit::Die()
{
    state.isDead = true;
    std::cout << "[Unit] Died.\n";
    if (HasComponent<TransformComponent>())
        GetComponent<TransformComponent>().scale = glm::vec3(0);
}