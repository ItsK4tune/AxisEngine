#include <render/logic/compute_shader.h>
#include <render/logic/shader.h>
#include <render/logic/texture_atlas.h>
#include <render/logic/video_decoder.h>
#include <render/logic/mesh.h>
#include <render/logic/static_batch_manager.h>
#include <render/logic/font.h>
#include <render/logic/particle_emitter.h>
#include <render/logic/shadow.h>
#include <render/logic/skybox.h>
#include <render/logic/ui_model.h>
#include <render/logic/renderer_initializer.h>
#include <render/interface/i_graphics_context.h>
#include <physics/strategy/bullet/bullet_debug_drawer.h>
#include <resource/logic/texture_cache.h>

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
