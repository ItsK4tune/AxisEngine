#pragma once

#include <cmath>
#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace axis_test
{
struct TestCase
{
    const char* name;
    void (*fn)();
};

struct Colors
{
    bool enabled = true;
    const char* reset = "\033[0m";
    const char* yellow = "\033[33m";
    const char* green = "\033[32m";
    const char* red = "\033[31m";
    const char* cyan = "\033[36m";
};

inline Colors& ConsoleColors()
{
    static Colors colors;
    return colors;
}

inline std::vector<TestCase>& Registry()
{
    static std::vector<TestCase> tests;
    return tests;
}

inline int& ExpectedErrorLogCount()
{
    static int count = 0;
    return count;
}

inline void ResetExpectedErrorLogs()
{
    ExpectedErrorLogCount() = 0;
}

inline void ExpectErrorLogs(int count)
{
    ExpectedErrorLogCount() += count;
}

inline int ConsumeExpectedErrorLogs()
{
    const int count = ExpectedErrorLogCount();
    ResetExpectedErrorLogs();
    return count;
}

struct Registrar
{
    Registrar(const char* name, void (*fn)())
    {
        Registry().push_back({name, fn});
    }
};

class Failure : public std::exception
{
public:
    explicit Failure(std::string message) : m_Message(std::move(message))
    {
    }

    const char* what() const noexcept override
    {
        return m_Message.c_str();
    }

private:
    std::string m_Message;
};

inline void Check(bool condition, const char* expression, const char* file, int line)
{
    if (condition)
        return;

    std::ostringstream ss;
    ss << file << ":" << line << " check failed: " << expression;
    throw Failure(ss.str());
}

inline bool Near(float a, float b, float epsilon = 0.0001f)
{
    return std::fabs(a - b) <= epsilon;
}
}  // namespace axis_test

#define AXIS_TEST_CONCAT_INNER(a, b) a##b
#define AXIS_TEST_CONCAT(a, b) AXIS_TEST_CONCAT_INNER(a, b)

#define AXIS_TEST_CASE(name)                                                                       \
    static void AXIS_TEST_CONCAT(axis_test_fn_, __LINE__)();                                       \
    namespace                                                                                      \
    {                                                                                              \
    axis_test::Registrar AXIS_TEST_CONCAT(axis_test_reg_, __LINE__)(name,                          \
                                                                    AXIS_TEST_CONCAT(axis_test_fn_, \
                                                                                     __LINE__));    \
    }                                                                                              \
    static void AXIS_TEST_CONCAT(axis_test_fn_, __LINE__)()

#define AXIS_CHECK(expression) axis_test::Check((expression), #expression, __FILE__, __LINE__)
#define AXIS_CHECK_NEAR(a, b, epsilon) AXIS_CHECK(axis_test::Near((a), (b), (epsilon)))
#define AXIS_EXPECT_ERROR_LOGS(count) axis_test::ExpectErrorLogs(count)
