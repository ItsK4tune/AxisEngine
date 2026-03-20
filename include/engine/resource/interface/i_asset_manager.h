#pragma once

#include <string>
#include <memory>
#include <vector>

/**
 * @brief Generic interface for all asset managers in AxisEngine.
 * 
 * Part of Phase 2 refactoring to standardize resource handling.
 */
template <typename T>
class IAssetManager
{
public:
    virtual ~IAssetManager() = default;

    /**
     * @brief Load an asset from a file path.
     * @param path The absolute or relative path to the asset.
     * @return A shared pointer to the loaded asset, or nullptr if loading failed.
     */
    virtual std::shared_ptr<T> Load(const std::string& path) = 0;

    /**
     * @brief Get a loaded asset by its name or path.
     */
    virtual std::shared_ptr<T> Get(const std::string& nameOrPath) = 0;

    /**
     * @brief Unload an asset.
     */
    virtual void Unload(const std::string& nameOrPath) = 0;

    /**
     * @brief Clear all assets from the manager.
     */
    virtual void Clear() = 0;

    /**
     * @brief Optional update step (e.g., for async loading or GC).
     */
    virtual void Update(float dt) {}
};
