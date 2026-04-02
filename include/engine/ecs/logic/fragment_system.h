#pragma once

#include <ecs/interface/i_update_system.h>
#include <string>

class FragmentSystem : public IUpdateSystem {
public:
    FragmentSystem();
    virtual ~FragmentSystem() = default;

    void Update(Scene& scene, float dt) override;
    
    SystemCategory GetCategory() const override { return SystemCategory::Update; }
    int GetPriority() const override { return 5; } // Run very early to instantiate entities for other systems
    std::string GetName() const override { return "FragmentSystem"; }
    
    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enabled) override { m_Enabled = enabled; }

private:
    bool m_Enabled = true;
};
