#pragma once

struct OptimizationConfig;

// Optional capability for systems whose runtime performance policy is driven
// by AppConfig. User-provided systems can implement this interface to join the
// same live configuration flow without Application knowing their concrete type.
class IOptimizationConfigurable
{
public:
    virtual ~IOptimizationConfigurable() = default;
    virtual void ApplyOptimizationConfig(const OptimizationConfig& config) = 0;
};
