#pragma once

class IGraphicsContext;

class RendererInitializer
{
public:
    static void Initialize(IGraphicsContext& context);
    static void Shutdown();
};
