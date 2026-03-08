#pragma once

#include <entt/entt.hpp>
#include <script/logic/scriptable.h>

class PhysicsScriptable : public virtual Scriptable {
public:
    virtual void OnCollisionEnter(entt::entity other) {}
    virtual void OnCollisionStay(entt::entity other) {}
    virtual void OnCollisionExit(entt::entity other) {}

    virtual void OnTriggerEnter(entt::entity other) {}
    virtual void OnTriggerStay(entt::entity other) {}
    virtual void OnTriggerExit(entt::entity other) {}
};