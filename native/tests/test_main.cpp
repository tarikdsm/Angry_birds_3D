#include "test_framework.hpp"

#include <exception>
#include <iostream>
#include <string>
#include <string_view>

int main(int argc, char** argv)
{
    std::string filter;
    if (argc != 1) {
        if (argc != 3 || std::string_view{argv[1]} != "--filter"
            || std::string_view{argv[2]}.empty()) {
            std::cerr << "usage: ninho_physics_tests [--filter <non-empty substring>]\n";
            return 2;
        }
        filter = argv[2];
    }

    int failures = 0;
    int selected = 0;
    for (const auto& test : ninho::test::registry()) {
        if (!filter.empty() && test.name.find(filter) == std::string::npos) {
            continue;
        }

        ++selected;
        try {
            test.run();
            std::cout << "[PASS] " << test.name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cout << "[FAIL] " << test.name << ": " << error.what() << '\n';
        } catch (...) {
            ++failures;
            std::cout << "[FAIL] " << test.name << ": unknown exception\n";
        }
    }

    if (selected == 0) {
        std::cerr << "no tests matched filter: " << filter << '\n';
        return 1;
    }

    return failures;
}
