#include <resource/unit/compute_shader.h>
#include <resource/unit/shader.h>
#include <render/unit/texture_atlas.h>
#include <render/logic/video_decoder.h>
#include <resource/unit/mesh.h>
#include <render/logic/static_batch_manager.h>
#include <resource/unit/font.h>
#include <render/logic/particle_emitter.h>
#include <render/unit/shadow.h>
#include <render/unit/skybox.h>
#include <resource/unit/ui_model.h>
#include <render/logic/renderer_initializer.h>
#include <render/interface/i_graphics_context.h>
#include <physics/strategy/bullet/bullet_debug_drawer.h>

void RendererInitializer::Initialize(IGraphicsContext &context)
{
    Mesh::SetManagers(&context.GetBufferManager(),
                      &context.GetTextureManager(),
                      &context.GetDrawContext());

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
