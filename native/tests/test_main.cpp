#include "test_framework.hpp"

#include <atomic>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <new>
#include <string>
#include <string_view>

namespace {

std::atomic<bool> count_allocations{};
std::atomic<std::size_t> allocation_count{};

void observe_allocation() noexcept
{
    if (count_allocations.load(std::memory_order_relaxed)) {
        allocation_count.fetch_add(1, std::memory_order_relaxed);
    }
}

}

void* operator new(std::size_t size)
{
    observe_allocation();
    if (void* allocation = std::malloc(size == 0 ? 1 : size)) {
        return allocation;
    }
    throw std::bad_alloc{};
}

void* operator new[](std::size_t size)
{
    return ::operator new(size);
}

void operator delete(void* allocation) noexcept
{
    std::free(allocation);
}

void operator delete[](void* allocation) noexcept
{
    ::operator delete(allocation);
}

void operator delete(void* allocation, std::size_t) noexcept
{
    ::operator delete(allocation);
}

void operator delete[](void* allocation, std::size_t) noexcept
{
    ::operator delete(allocation);
}

namespace ninho::test {

void begin_allocation_counting() noexcept
{
    allocation_count.store(0, std::memory_order_relaxed);
    count_allocations.store(true, std::memory_order_relaxed);
}

std::size_t end_allocation_counting() noexcept
{
    count_allocations.store(false, std::memory_order_relaxed);
    return allocation_count.load(std::memory_order_relaxed);
}

}

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
