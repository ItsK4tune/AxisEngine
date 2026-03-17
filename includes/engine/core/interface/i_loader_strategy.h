#pragma once
#include <string>
#include <core/unit/engine_context.h>

/**
 * @brief Interface for unified loading strategies.
 */
class ILoaderStrategy {
public:
    virtual ~ILoaderStrategy() = default;

    /**
     * @brief Executes the loading logic for a specific file path.
     * @param path File path to load.
     * @param ctx Engine context providing necessary dependencies.
     */
    virtual bool Load(const std::string& path, EngineContext ctx) = 0;

    /**
     * @brief Returns the unique identifier for this strategy (e.g., "INPUT", "PHYSICS").
     */
    virtual const char* GetName() const = 0;
};
