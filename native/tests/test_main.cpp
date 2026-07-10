#include "test_framework.hpp"

#include <exception>
#include <iostream>
#include <string>
#include <string_view>

int main(int argc, char** argv)
{
    std::string filter;
    if (argc == 3 && std::string_view{argv[1]} == "--filter") {
        filter = argv[2];
    }

    int failures = 0;
    for (const auto& test : ninho::test::registry()) {
        if (!filter.empty() && test.name.find(filter) == std::string::npos) {
            continue;
        }

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

    return failures;
}
