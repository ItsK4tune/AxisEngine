#pragma once

#include <resource/unit/shader.h>
#include <string>
#include <vector>

class IBufferManager;
class IDrawContext;
class ITextureManager;

class Skybox
{
public:
    Skybox();
    ~Skybox();

    void Draw(Shader& shader);
    // Returns true only when all six source faces were loaded. The cubemap may
    // still be rendered with explicit fallback faces when false is returned.
    bool LoadCubemap(const std::vector<std::string>& faces);
    unsigned int GetTextureID() const
    {
        return m_TextureID;
    }

    std::string GetName() const
    {
        return m_Name;
    }
    void SetName(const std::string& name)
    {
        m_Name = name;
    }

    static void SetManagers(IBufferManager& bufferManager, ITextureManager& textureManager, IDrawContext& drawContext);
    static void ClearManagers();

    unsigned int GetIrradianceMap() const
    {
        return m_IrradianceMap;
    }
    unsigned int GetPrefilterMap() const
    {
        return m_PrefilterMap;
    }
    unsigned int GetBrdfLUT() const
    {
        return m_BrdfLUT;
    }

    void SetIBLMaps(unsigned int irradiance, unsigned int prefilter, unsigned int brdf)
    {
        m_IrradianceMap = irradiance;
        m_PrefilterMap = prefilter;
        m_BrdfLUT = brdf;
    }

private:
    static IBufferManager* s_BufferManager;
    static ITextureManager* s_TextureManager;
    static IDrawContext* s_DrawContext;

    void Initialize();

    static IBufferManager& GetBufferManager();
    static ITextureManager& GetTextureManager();
    static IDrawContext& GetDrawContext();
    unsigned int m_VAO = 0;
    unsigned int m_VBO = 0;
    unsigned int m_TextureID = 0;

    unsigned int m_IrradianceMap = 0;
    unsigned int m_PrefilterMap = 0;
    unsigned int m_BrdfLUT = 0;
    std::string m_Name;
};
