// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Marcos Quirino Deida
// Tests only (feature count-cpp-new). Replaces the global operator new and delete so that every
// allocation of the model goes through Rust's allocator, where the conformance kit counts it.
// Without this the kit's allocation check cannot see the C++ side. malloc called directly is not seen.
#include <cstddef>
#include <new>

extern "C" void* bc_rust_alloc(std::size_t size) noexcept;
extern "C" void bc_rust_free(void* p) noexcept;

void* operator new(std::size_t n)
{
    void* p = bc_rust_alloc(n);
    if (!p) throw std::bad_alloc();
    return p;
}
void* operator new[](std::size_t n) { return operator new(n); }
void* operator new(std::size_t n, const std::nothrow_t&) noexcept { return bc_rust_alloc(n); }
void* operator new[](std::size_t n, const std::nothrow_t&) noexcept { return bc_rust_alloc(n); }
void operator delete(void* p) noexcept { bc_rust_free(p); }
void operator delete[](void* p) noexcept { bc_rust_free(p); }
void operator delete(void* p, std::size_t) noexcept { bc_rust_free(p); }
void operator delete[](void* p, std::size_t) noexcept { bc_rust_free(p); }
