#pragma once

#include <render/logic/shader.h>
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
    void LoadCubemap(const std::vector<std::string>& faces);
    unsigned int GetTextureID() const { return m_TextureID; }

    static void SetManagers(IBufferManager& bufferManager, ITextureManager& textureManager, IDrawContext& drawContext);

private:
    unsigned int m_VAO = 0;
    unsigned int m_VBO = 0;
    unsigned int m_TextureID = 0;

    // IBL Maps
    unsigned int m_IrradianceMap = 0;
    unsigned int m_PrefilterMap = 0;
    unsigned int m_BrdfLUT = 0;

public:
    unsigned int GetIrradianceMap() const { return m_IrradianceMap; }
    unsigned int GetPrefilterMap() const { return m_PrefilterMap; }
    unsigned int GetBrdfLUT() const { return m_BrdfLUT; }

    void SetIBLMaps(unsigned int irradiance, unsigned int prefilter, unsigned int brdf) {
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
};