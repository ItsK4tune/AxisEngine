#pragma once

#include <rendering/types/graphics_enums.h>
#include <rendering/types/buffer_types.h>
#include <rendering/types/texture_types.h>
#include <rendering/types/render_state_types.h>
#include <rendering/types/framebuffer_types.h>
#include <rendering/types/graphics_query_types.h>
#include <rendering/types/gpu_handle.h>
#include <rendering/types/graphics_enums.h>
#include <rendering/types/buffer_types.h>
#include <rendering/types/texture_types.h>
#include <rendering/types/render_state_types.h>
#include <rendering/types/framebuffer_types.h>
#include <rendering/types/graphics_query_types.h>
#include <rendering/types/gpu_handle.h>

class IQueryManager {
public:
    virtual ~IQueryManager() = default;

    virtual uint32_t GenQuery() = 0;
    virtual void DeleteQuery(uint32_t queryId) = 0;

    virtual void BeginQuery(Graphics::QueryType type, uint32_t queryId) = 0;
    virtual void EndQuery(Graphics::QueryType type) = 0;

    virtual bool IsResultAvailable(uint32_t queryId) = 0;
    virtual uint32_t GetQueryResult(uint32_t queryId) = 0;
};
