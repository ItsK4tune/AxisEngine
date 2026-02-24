#pragma once

class RenderSystem;
class PhysicsSystem;
class AudioSystem;
class UIRenderSystem;
class ScriptableSystem;
class ParticleSystem;
class SkyboxRenderSystem;
class AnimationSystem;
class VideoSystem;
class PostProcessPipeline;

struct SystemContext
{
    RenderSystem&        render;
    PhysicsSystem&       physics;
    AudioSystem&         audio;
    UIRenderSystem&      ui;
    ScriptableSystem&    script;
    ParticleSystem&      particle;
    SkyboxRenderSystem&  skybox;
    AnimationSystem&     animation;
    VideoSystem&         video;
    PostProcessPipeline& postProcess;
};
