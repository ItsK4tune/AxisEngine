#pragma once

#include <render/interface/i_query_manager.h>

class OpenGLQueryManager : public IQueryManager
{
public:
    uint32_t GenQuery() override;
    void DeleteQuery(uint32_t queryId) override;

    void BeginQuery(QueryType type, uint32_t queryId) override;
    void EndQuery(QueryType type) override;

    bool IsResultAvailable(uint32_t queryId) override;
    uint32_t GetQueryResult(uint32_t queryId) override;
};
