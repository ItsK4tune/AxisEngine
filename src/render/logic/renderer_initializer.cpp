#include <render/logic/renderer_initializer.h>
#include <render/interface/i_graphics_context.h>
#include <render/logic/particle_emitter.h>
#include <render/logic/video_decoder.h>
#include <render/unit/shadow.h>
#include <render/unit/skybox.h>
#include <resource/unit/compute_shader.h>
#include <resource/unit/font.h>
#include <resource/unit/mesh.h>
#include <resource/unit/shader.h>
#include <resource/unit/ui_model.h>

void RendererInitializer::Initialize(IGraphicsContext& context)
{
    Mesh::SetManagers(&context.GetBufferManager(), &context.GetTextureManager(), &context.GetDrawContext());

    Shadow::SetManagers(&context.GetRenderTargetManager(), &context.GetTextureManager(), &context.GetDrawContext());
    Skybox::SetManagers(context.GetBufferManager(), context.GetTextureManager(), context.GetDrawContext());
    VideoDecoder::SetTextureManager(context.GetTextureManager());
    VideoDecoder::SetBufferManager(context.GetBufferManager());
    ParticleEmitter::SetManagers(context.GetBufferManager(), context.GetTextureManager(), context.GetDrawContext());
    Font::SetTextureManager(context.GetTextureManager());
    UIModel::SetManagers(context.GetBufferManager(), context.GetTextureManager(), context.GetDrawContext());
}

void RendererInitializer::Shutdown()
{
    UIModel::ClearManagers();
    Font::ClearTextureManager();
    ParticleEmitter::ClearManagers();
    VideoDecoder::ClearTextureManager();
    VideoDecoder::ClearBufferManager();
    Skybox::ClearManagers();
    Shadow::SetManagers(nullptr, nullptr, nullptr);
    Mesh::SetManagers(nullptr, nullptr, nullptr);
}
