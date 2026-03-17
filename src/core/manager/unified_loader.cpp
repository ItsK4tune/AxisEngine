#include <core/manager/unified_loader.h>
#include <core/logic/logger.h>

UnifiedLoader& UnifiedLoader::Instance() {
    static UnifiedLoader instance;
    return instance;
}

void UnifiedLoader::Register(std::unique_ptr<ILoaderStrategy> strategy) {
    if (!strategy) return;
    std::string name = strategy->GetName();
    m_Strategies[name] = std::move(strategy);
    LOGGER_INFO("UnifiedLoader") << "Registered loading strategy: " << name;
}

bool UnifiedLoader::Load(const std::string& type, const std::string& path, EngineContext ctx) {
    auto it = m_Strategies.find(type);
    if (it != m_Strategies.end()) {
        LOGGER_INFO("UnifiedLoader") << "Dispatching '" << path << "' to strategy: " << type;
        return it->second->Load(path, ctx);
    }
    
    LOGGER_ERROR("UnifiedLoader") << "No strategy found for type: " << type;
    return false;
}

std::vector<std::string> UnifiedLoader::GetRegisteredTypes() const {
    std::vector<std::string> types;
    for (const auto& pair : m_Strategies) {
        types.push_back(pair.first);
    }
    return types;
}
