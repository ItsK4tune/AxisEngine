// #pragma once
// #include <string>

// class Unit;

// enum class SkillType { PASSIVE, ACTIVE };

// class Skill {
// public:
//     virtual ~Skill() = default;
//     virtual std::string GetName() const = 0;
//     virtual SkillType GetType() const = 0;

//     virtual void OnTurnStart(Unit* owner) {}
//     virtual int OnCalculateDamage(Unit* owner, Unit* target, int baseDamage) { return baseDamage; }
    
//     virtual int GetCost() { return 0; }
//     virtual int GetRange() { return 1; }
//     virtual void Execute(Unit* owner, Unit* target) {}
// };