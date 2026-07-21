#pragma once

#include <core/type/app_config.h>
#include <string>
#include <vector>

struct ConfigValidationIssue
{
    std::string field;
    std::string message;
};

struct ConfigValidationResult
{
    AppConfig config;
    std::vector<ConfigValidationIssue> issues;

    bool WasSanitized() const
    {
        return !issues.empty();
    }
};

struct ConfigValidationPolicy
{
    bool allowUncompiledGraphicsBackend = false;
    bool allowUncompiledPhysicsBackend = false;
    bool allowUncompiledAudioBackend = false;
};

// Produces a runtime-safe configuration without mutating the caller's value.
// Every correction is reported so tools can explain the effective value.
ConfigValidationResult ValidateAndSanitizeConfig(const AppConfig& config, const ConfigValidationPolicy& policy = {});
