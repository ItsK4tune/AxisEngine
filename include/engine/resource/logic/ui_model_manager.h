#pragma once

#include <resource/unit/ui_model.h>
#include <memory>
#include <string>
#include <unordered_map>

class UIModelManager
{
public:
    void Create(const std::string& name, UIType type)
    {
        m_UIModels[name] = std::make_shared<UIModel>(type);
    }

    std::shared_ptr<UIModel> Get(const std::string& name)
    {
        auto it = m_UIModels.find(name);
        if (it != m_UIModels.end())
        {
            return it->second;
        }
        return nullptr;
    }

    bool Has(const std::string& name) const
    {
        return m_UIModels.find(name) != m_UIModels.end();
    }

    void Clear()
    {
        m_UIModels.clear();
    }

private:
    std::unordered_map<std::string, std::shared_ptr<UIModel>> m_UIModels;
};
