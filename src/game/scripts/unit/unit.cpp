#include <game/scripts/unit/unit.h>
#include <engine/core/application.h>

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

void Unit::InitFromFile(const std::string &path)
{
    MeshRendererComponent *renderer = nullptr;
    if (HasComponent<MeshRendererComponent>())
        renderer = &GetComponent<MeshRendererComponent>();

    loader.Load(path, stats, m_App->GetResourceManager(), renderer);
}

void Unit::MoveByPath(const std::vector<HexCoord> &path)
{
    movement.StartPath(path);
    state.gridPos = path.back();
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

void Unit::Die()
{
    state.isDead = true;
    std::cout << "[Unit] Died.\n";
    if (HasComponent<TransformComponent>())
        GetComponent<TransformComponent>().scale = glm::vec3(0);
}