#include <graphics/core/compute_shader.h>
#include <graphics/core/shader.h>
#include <graphics/core/texture_atlas.h>
#include <graphics/core/video_decoder.h>
#include <graphics/geometry/mesh.h>
#include <graphics/geometry/static_batch_manager.h>
#include <graphics/renderer/font.h>
#include <graphics/renderer/particle_emitter.h>
#include <graphics/renderer/shadow.h>
#include <graphics/renderer/skybox.h>
#include <graphics/renderer/ui_model.h>
#include <graphics/renderer_initializer.h>
#include <graphics/interfaces/i_graphics_context.h>
#include <physics/backends/bullet_debug_drawer.h>
#include <resource/texture_cache.h>

void RendererInitializer::Initialize(IGraphicsContext &context)
{
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
    TextureAtlas::SetTextureManager(context.GetTextureManager());
    UIModel::SetManagers(context.GetBufferManager(),
                         context.GetTextureManager(),
                         context.GetDrawContext());
    StaticBatchManager::SetManagers(context.GetBufferManager(),
                                    context.GetDrawContext());
    BulletDebugDrawer::SetManagers(context.GetBufferManager(),
                                   context.GetDrawContext());
}
