#pragma once

#include <graphics/interfaces/graphics_types.h>
#include <graphics/interfaces/graphics_types.h>

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
