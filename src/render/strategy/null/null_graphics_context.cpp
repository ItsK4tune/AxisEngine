#include <render/strategy/null/null_graphics_context.h>
#include <core/logic/backend_factory_registry.h>

namespace axis::backend
{
void RegisterNullGraphicsBackendFactories()
{
    BackendFactoryRegistry::RegisterGraphics(
        GraphicsBackend::Null,
        [](const AppConfig&) {
            return std::make_unique<NullGraphicsContext>();
        }
    );
}
}  // namespace axis::backend
