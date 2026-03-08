#pragma once

class AnimationSystem;
class AudioSystem;
class ParticleSystem;
class PhysicsSystem;
class PostProcessPipeline;
class RenderSystem;
class ScriptableSystem;
class SkyboxRenderSystem;
class UIRenderSystem;
class VideoSystem;

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