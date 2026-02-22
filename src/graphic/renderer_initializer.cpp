#include <graphic/renderer_initializer.h>

#include <interface/graphic/i_graphics_context.h>
#include <graphic/core/shader.h>
#include <graphic/geometry/mesh.h>
#include <resource/texture_cache.h>
#include <graphic/renderer/shadow.h>
#include <graphic/core/video_decoder.h>
#include <graphic/renderer/skybox.h>
#include <graphic/renderer/particle_emitter.h>
#include <graphic/renderer/font.h>
#include <graphic/core/compute_shader.h>
#include <graphic/core/texture_atlas.h>
#include <graphic/renderer/ui_model.h>
#include <graphic/geometry/static_batch_manager.h>
#include <physic/backends/bullet_debug_drawer.h>

void RendererInitializer::Initialize(IGraphicsContext &context)
{
    Shader::SetShaderManager(&context.GetShaderManager());
    Mesh::SetManagers(&context.GetBufferManager(),
                      &context.GetTextureManager(),
                      &context.GetDrawContext());

    TextureCache::SetTextureManager(&context.GetTextureManager());
    Shadow::SetManagers(&context.GetRenderTargetManager(),
                        &context.GetTextureManager(),
                        &context.GetDrawContext());
    Skybox::SetManagers(context.GetBufferManager(),
                        context.GetTextureManager(),
                        context.GetDrawContext());
    VideoDecoder::SetTextureManager(context.GetTextureManager());
    ParticleEmitter::SetManagers(context.GetBufferManager(),
                                 context.GetTextureManager(),
                                 context.GetDrawContext());
    Font::SetTextureManager(context.GetTextureManager());
    ComputeShader::SetShaderManager(context.GetShaderManager());
    TextureAtlas::SetTextureManager(context.GetTextureManager());
    UIModel::SetManagers(context.GetBufferManager(),
                         context.GetTextureManager(),
                         context.GetDrawContext());
    StaticBatchManager::SetManagers(context.GetBufferManager(),
                                    context.GetDrawContext());
    BulletDebugDrawer::SetManagers(context.GetBufferManager(),
                                   context.GetDrawContext());
}
