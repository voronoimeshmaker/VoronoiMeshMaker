// ============================================================================
// File: allocation_failure.hpp
// Description: One-shot allocation failure injection for public reader tests.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================
#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <new>

namespace vmm::test {
inline thread_local std::size_t allocation_countdown = std::numeric_limits<std::size_t>::max();
inline thread_local bool allocation_failed = false;
inline void fail_allocation_after(std::size_t count) {
    allocation_countdown = count;
    allocation_failed = false;
}
inline void stop_allocation_failure() { allocation_countdown = std::numeric_limits<std::size_t>::max(); }
}

[[gnu::noinline]] void* operator new(std::size_t size) {
    auto& count = vmm::test::allocation_countdown;
    if (count != std::numeric_limits<std::size_t>::max()) {
        if (count == 0) {
            vmm::test::stop_allocation_failure();
            vmm::test::allocation_failed = true;
            throw std::bad_alloc();
        }
        --count;
    }
    if (void* memory = std::malloc(size == 0 ? 1 : size)) return memory;
    throw std::bad_alloc();
}
[[gnu::noinline]] void operator delete(void* memory) noexcept { std::free(memory); }
[[gnu::noinline]] void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
[[gnu::noinline]] void* operator new[](std::size_t size) { return ::operator new(size); }
[[gnu::noinline]] void operator delete[](void* memory) noexcept { ::operator delete(memory); }
[[gnu::noinline]] void operator delete[](void* memory, std::size_t) noexcept { ::operator delete(memory); }
