#pragma once

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace ninho::simulation::test {

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

}

#define NINHO_SIM_JOIN_INNER(a, b) a##b
#define NINHO_SIM_JOIN(a, b) NINHO_SIM_JOIN_INNER(a, b)
#define NINHO_SIM_TEST(name) NINHO_SIM_TEST_IMPL(name, __COUNTER__)
#define NINHO_SIM_TEST_IMPL(name, n)                                                              \
    static void NINHO_SIM_JOIN(ninho_simulation_test_, n)();                                      \
    static ::ninho::simulation::test::Registrar NINHO_SIM_JOIN(ninho_simulation_reg_, n)(         \
        name, &NINHO_SIM_JOIN(ninho_simulation_test_, n));                                        \
    static void NINHO_SIM_JOIN(ninho_simulation_test_, n)()
#define NINHO_SIM_REQUIRE(expression)                                                              \
    do {                                                                                           \
        if (!(expression)) {                                                                       \
            ::ninho::simulation::test::fail(                                                       \
                __FILE__, __LINE__, "require failed: " #expression);                             \
        }                                                                                          \
    } while (false)
