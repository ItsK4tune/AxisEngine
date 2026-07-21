#pragma once

#include <render/type/graphics_types.h>

class IQueryManager
{
public:
    virtual ~IQueryManager() = default;

    virtual uint32_t GenQuery() = 0;
    virtual void DeleteQuery(uint32_t queryId) = 0;

    virtual void BeginQuery(QueryType type, uint32_t queryId) = 0;
    virtual void EndQuery(QueryType type) = 0;

    virtual bool IsResultAvailable(uint32_t queryId) = 0;
    virtual uint32_t GetQueryResult(uint32_t queryId) = 0;
    virtual uint64_t GetQueryResult64(uint32_t queryId)
    {
        return GetQueryResult(queryId);
    }
    virtual bool SupportsQuery(QueryType) const
    {
        return false;
    }
};
