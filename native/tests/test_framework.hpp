#pragma once

#include <cmath>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace ninho::test {

struct Case {
    std::string name;
    void (*run)();
};

inline std::vector<Case>& registry()
{
    static std::vector<Case> value;
    return value;
}

struct Registrar {
    Registrar(const char* name, void (*run)())
    {
        registry().push_back({name, run});
    }
};

[[noreturn]] inline void fail(const char* file, int line, const std::string& message)
{
    std::ostringstream out;
    out << file << ':' << line << ": " << message;
    throw std::runtime_error(out.str());
}

inline void require_near(
    const char* file,
    int line,
    const char* actual_expression,
    const char* expected_expression,
    const char* epsilon_expression,
    double actual,
    double expected,
    double epsilon)
{
    if (!std::isfinite(actual) || !std::isfinite(expected) || !std::isfinite(epsilon)) {
        std::ostringstream message;
        message << "near requires finite values: " << actual_expression << '=' << actual
                << ", " << expected_expression << '=' << expected << ", epsilon=" << epsilon;
        fail(file, line, message.str());
    }
    if (epsilon < 0.0) {
        std::ostringstream message;
        message << "epsilon must be non-negative: " << epsilon_expression << '=' << epsilon;
        fail(file, line, message.str());
    }
    if (std::abs(actual - expected) > epsilon) {
        std::ostringstream message;
        message << actual_expression << '=' << actual << ", expected " << expected
                << " +/- " << epsilon;
        fail(file, line, message.str());
    }
}

}

#define NINHO_JOIN_INNER(a, b) a##b
#define NINHO_JOIN(a, b) NINHO_JOIN_INNER(a, b)
#define NINHO_TEST(name) NINHO_TEST_IMPL(name, __COUNTER__)
#define NINHO_TEST_IMPL(name, n)                                                                     \
    static void NINHO_JOIN(ninho_test_, n)();                                                        \
    static ::ninho::test::Registrar NINHO_JOIN(ninho_reg_, n)(name, &NINHO_JOIN(ninho_test_, n));   \
    static void NINHO_JOIN(ninho_test_, n)()
#define NINHO_REQUIRE(expr)                                                                          \
    do {                                                                                             \
        if (!(expr)) {                                                                               \
            ::ninho::test::fail(__FILE__, __LINE__, "require failed: " #expr);                     \
        }                                                                                            \
    } while (false)
#define NINHO_REQUIRE_NEAR(actual, expected, epsilon)                                                \
    do {                                                                                             \
        const double a_ = (actual), e_ = (expected), d_ = (epsilon);                                \
        ::ninho::test::require_near(                                                                 \
            __FILE__, __LINE__, #actual, #expected, #epsilon, a_, e_, d_);                          \
    } while (false)
