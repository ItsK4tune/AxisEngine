#include <graphic/backends/opengl_query_manager.h>
#include <graphic/backends/opengl_translator.h>
#include <glad/glad.h>

uint32_t OpenGLQueryManager::GenQuery() {
    uint32_t queryId;
    glGenQueries(1, &queryId);
    return queryId;
}

void OpenGLQueryManager::DeleteQuery(uint32_t queryId) {
    glDeleteQueries(1, &queryId);
}

void OpenGLQueryManager::BeginQuery(Graphics::QueryType type, uint32_t queryId) {
    glBeginQuery(GLTranslator::ToGL(type), queryId);
}

void OpenGLQueryManager::EndQuery(Graphics::QueryType type) {
    glEndQuery(GLTranslator::ToGL(type));
}

bool OpenGLQueryManager::IsResultAvailable(uint32_t queryId) {
    int available = 0;
    glGetQueryObjectiv(queryId, GL_QUERY_RESULT_AVAILABLE, &available);
    return available != 0;
}

uint32_t OpenGLQueryManager::GetQueryResult(uint32_t queryId) {
    uint32_t result = 0;
    glGetQueryObjectuiv(queryId, GL_QUERY_RESULT, &result);
    return result;
}
