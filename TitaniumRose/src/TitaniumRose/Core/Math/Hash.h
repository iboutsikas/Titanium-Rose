#pragma once
#include <limits>
#include <type_traits>
#include <cstdint>
#include <functional>

// Taken from https://stackoverflow.com/a/50978188
#if 1
namespace Roses {
    template<typename T>
    T xorshift(const T& n, int i) {
        return n ^ (n >> i);
    }

    uint32_t distribute(const uint32_t& n) {
        uint32_t p = 0x55555555ul; // pattern of alternating 0 and 1
        uint32_t c = 3423571495ul; // random uneven integer constant; 
        return c * xorshift(p * xorshift(n, 16), 16);
    }

    uint64_t hash(const uint64_t& n) {
        uint64_t p = 0x5555555555555555;     // pattern of alternating 0 and 1
        uint64_t c = 17316035218449499591ull;// random uneven integer constant; 
        return c * xorshift(p * xorshift(n, 32), 32);
    }

    // if c++20 rotl is not available:
    template <typename T, typename S>
    typename std::enable_if<std::is_unsigned<T>::value, T>::type
        constexpr rotl(const T n, const S i) {
        const T m = (std::numeric_limits<T>::digits - 1);
        const T c = i & m;
        return (n << c) | (n >> ((T(0) - c) & m)); // this is usually recognized by the compiler to mean rotation, also c++20 now gives us rotl directly
    }

    template <class T>
    inline size_t hash_combine(std::size_t& seed, const T& v)
    {
        size_t h = std::hash<T>{}(v);
        return rotl(seed, std::numeric_limits<size_t>::digits / 3) ^ distribute(h);
    }
}
#endif