#include <rendering/core/compute_shader.h>
#include <rendering/core/shader.h>
#include <rendering/core/texture_atlas.h>
#include <rendering/core/video_decoder.h>
#include <rendering/geometry/mesh.h>
#include <rendering/geometry/static_batch_manager.h>
#include <rendering/renderer/font.h>
#include <rendering/renderer/particle_emitter.h>
#include <rendering/renderer/shadow.h>
#include <rendering/renderer/skybox.h>
#include <rendering/renderer/ui_model.h>
#include <rendering/renderer_initializer.h>
#include <rendering/interfaces/i_graphics_context.h>
#include <systems/physics/backends/bullet_debug_drawer.h>
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
