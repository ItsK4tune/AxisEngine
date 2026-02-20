#pragma once

#include <vector>
#include <string>
#include <graphic/core/shader.h>

class IBufferManager;
class ITextureManager;
class IDrawContext;

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

    static IBufferManager* s_BufferManager;
    static ITextureManager* s_TextureManager;
    static IDrawContext* s_DrawContext;

    void Init();

    static IBufferManager& GetBufferManager();
    static ITextureManager& GetTextureManager();
    static IDrawContext& GetDrawContext();
};
