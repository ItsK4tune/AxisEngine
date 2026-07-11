#pragma once

#include <entt/entt.hpp>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

enum class SceneValidationSeverity
{
    Info,
    Warning,
    Error,
    Fatal
};

struct SceneValidationIssue
{
    SceneValidationSeverity severity = SceneValidationSeverity::Info;
    std::string code;
    entt::entity entity = entt::null;
    std::string component;
    std::string message;
};

struct SceneRenderCapabilities
{
    int directionalShadowLimit = 16;
    int pointShadowLimit = 16;
    int spotShadowLimit = 16;
    bool singleDirectionalShadow = false;
};

struct SceneValidationOptions
{
    std::vector<entt::entity> entityScope;
    bool deepValidation = false;
    bool validateRenderCapabilities = false;
    SceneRenderCapabilities renderCapabilities;
};

struct SceneValidationResult
{
    std::vector<SceneValidationIssue> issues;

    void Add(SceneValidationSeverity severity, std::string code, entt::entity entity, std::string component,
             std::string message)
    {
        issues.push_back({severity, std::move(code), entity, std::move(component), std::move(message)});
    }

    void Merge(SceneValidationResult other)
    {
        issues.insert(issues.end(), std::make_move_iterator(other.issues.begin()),
                      std::make_move_iterator(other.issues.end()));
    }

    bool HasErrors() const
    {
        for (const auto& issue : issues)
        {
            if (issue.severity == SceneValidationSeverity::Error || issue.severity == SceneValidationSeverity::Fatal)
                return true;
        }
        return false;
    }

    bool HasFatalErrors() const
    {
        for (const auto& issue : issues)
        {
            if (issue.severity == SceneValidationSeverity::Fatal)
                return true;
        }
        return false;
    }

    bool IsValid() const
    {
        return !HasErrors();
    }
};
