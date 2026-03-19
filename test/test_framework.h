#pragma once

#include <iostream>
#include <string>
#include <functional>
#include <vector>
#include <sstream>

/**
 * @brief Minimal unit test framework for AxisEngine.
 * 
 * No external dependencies. Simply include this header and use the macros:
 *   TEST_CASE(name) { ... }
 *   REQUIRE(expr)
 *   REQUIRE_THROWS(expr)
 *   SECTION(name) { ... }
 */

namespace axis_test {

struct TestResult {
    std::string name;
    bool passed;
    std::string message;
};

class TestRunner {
public:
    static TestRunner& Instance() {
        static TestRunner runner;
        return runner;
    }

    void AddTest(const std::string& name, std::function<void()> fn) {
        m_Tests.push_back({name, fn});
    }

    int Run() {
        int passed = 0, failed = 0;
        std::cout << "\n===== AxisEngine Unit Tests =====\n\n";

        for (auto& [name, fn] : m_Tests) {
            try {
                fn();
                std::cout << "  [PASS] " << name << "\n";
                passed++;
            } catch (const std::exception& e) {
                std::cout << "  [FAIL] " << name << "\n";
                std::cout << "         " << e.what() << "\n";
                failed++;
            }
        }

        std::cout << "\n---------------------------------\n";
        std::cout << "  Total: " << (passed + failed)
                  << " | Passed: " << passed
                  << " | Failed: " << failed << "\n";
        std::cout << "=================================\n\n";

        return failed > 0 ? 1 : 0;
    }

private:
    std::vector<std::pair<std::string, std::function<void()>>> m_Tests;
};

struct TestAutoRegister {
    TestAutoRegister(const std::string& name, std::function<void()> fn) {
        TestRunner::Instance().AddTest(name, fn);
    }
};

class AssertionError : public std::runtime_error {
public:
    AssertionError(const std::string& msg) : std::runtime_error(msg) {}
};

inline void Require(bool condition, const char* expr, const char* file, int line) {
    if (!condition) {
        std::ostringstream ss;
        ss << "REQUIRE failed: " << expr << " (" << file << ":" << line << ")";
        throw AssertionError(ss.str());
    }
}

inline void RequireEqual(const auto& actual, const auto& expected, const char* exprA, const char* exprB, const char* file, int line) {
    if (actual != expected) {
        std::ostringstream ss;
        ss << "REQUIRE_EQUAL failed: " << exprA << " == " << exprB
           << " (actual: " << actual << ", expected: " << expected << ")"
           << " (" << file << ":" << line << ")";
        throw AssertionError(ss.str());
    }
}

} // namespace axis_test

#define TEST_CASE(name) \
    static void AXIS_TEST_##name(); \
    static axis_test::TestAutoRegister AXIS_REG_##name(#name, AXIS_TEST_##name); \
    static void AXIS_TEST_##name()

#define REQUIRE(expr) \
    axis_test::Require((expr), #expr, __FILE__, __LINE__)

#define REQUIRE_EQUAL(a, b) \
    axis_test::RequireEqual((a), (b), #a, #b, __FILE__, __LINE__)

#define REQUIRE_THROWS(expr) \
    do { \
        bool threw = false; \
        try { expr; } catch (...) { threw = true; } \
        axis_test::Require(threw, "Expected exception from: " #expr, __FILE__, __LINE__); \
    } while(0)

#define REQUIRE_NULL(ptr) \
    axis_test::Require((ptr) == nullptr, #ptr " should be null", __FILE__, __LINE__)

#define REQUIRE_NOT_NULL(ptr) \
    axis_test::Require((ptr) != nullptr, #ptr " should not be null", __FILE__, __LINE__)
