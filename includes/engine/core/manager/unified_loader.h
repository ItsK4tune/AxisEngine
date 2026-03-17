#pragma once

#include <core/interface/i_loader_strategy.h>
#include <core/unit/engine_context.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

/**
 * @brief Central registry and dispatcher for all asset loading strategies.
 */
class UnifiedLoader {
public:
    /**
     * @brief Access the singleton instance.
     */
    static UnifiedLoader& Instance();

    /**
     * @brief Registers a new loading strategy.
     */
    void Register(std::unique_ptr<ILoaderStrategy> strategy);

    /**
     * @brief Dispatches a load request to the appropriate strategy.
     */
    bool Load(const std::string& type, const std::string& path, EngineContext ctx);
    
    /**
     * @brief Returns a list of all registered strategy names.
     */
    std::vector<std::string> GetRegisteredTypes() const;

private:
    UnifiedLoader() = default;
    
    std::unordered_map<std::string, std::unique_ptr<ILoaderStrategy>> m_Strategies;
};
