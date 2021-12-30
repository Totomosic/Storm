/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2021 The Stockfish developers (see AUTHORS file)

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// Constants used in NNUE evaluation function

#pragma once

#ifdef EMSCRIPTEN
#define USE_SSE2 1
#else
#define USE_SSE2 1
#endif

#include <cstring>
#include <iostream>
#include <optional>

// #include "../misc.h"  // for IsLittleEndian

#if defined(USE_AVX2)
#include <immintrin.h>

#elif defined(USE_SSE41)
#include <smmintrin.h>

#elif defined(USE_SSSE3)
#include <tmmintrin.h>

#elif defined(USE_SSE2)
#include <emmintrin.h>

#elif defined(USE_MMX)
#include <mmintrin.h>

#elif defined(USE_NEON)
#include <arm_neon.h>
#endif

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

namespace Storm::NNUE
{

    /// std_aligned_alloc() is our wrapper for systems where the c++17 implementation
    /// does not guarantee the availability of aligned_alloc(). Memory allocated with
    /// std_aligned_alloc() must be freed with std_aligned_free().

    inline void* std_aligned_alloc(size_t alignment, size_t size)
    {

#if defined(POSIXALIGNEDALLOC)
        void* mem;
        return posix_memalign(&mem, alignment, size) ? nullptr : mem;
#elif defined(_WIN32)
        return _mm_malloc(size, alignment);
#else
        return aligned_alloc(alignment, size);
#endif
    }

    inline void std_aligned_free(void* ptr)
    {

#if defined(POSIXALIGNEDALLOC)
        free(ptr);
#elif defined(_WIN32)
        _mm_free(ptr);
#else
        free(ptr);
#endif
    }

#if defined(_WIN32)

    inline void* aligned_large_pages_alloc_windows(size_t allocSize)
    {

#if !defined(_WIN64)
        return nullptr;
#else

        HANDLE hProcessToken {};
        LUID luid {};
        void* mem = nullptr;

        const size_t largePageSize = GetLargePageMinimum();
        if (!largePageSize)
            return nullptr;

        // We need SeLockMemoryPrivilege, so try to enable it for the process
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hProcessToken))
            return nullptr;

        if (LookupPrivilegeValue(NULL, SE_LOCK_MEMORY_NAME, &luid))
        {
            TOKEN_PRIVILEGES tp {};
            TOKEN_PRIVILEGES prevTp {};
            DWORD prevTpLen = 0;

            tp.PrivilegeCount = 1;
            tp.Privileges[0].Luid = luid;
            tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            // Try to enable SeLockMemoryPrivilege. Note that even if AdjustTokenPrivileges() succeeds,
            // we still need to query GetLastError() to ensure that the privileges were actually obtained.
            if (AdjustTokenPrivileges(hProcessToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), &prevTp, &prevTpLen) &&
                GetLastError() == ERROR_SUCCESS)
            {
                // Round up size to full pages and allocate
                allocSize = (allocSize + largePageSize - 1) & ~size_t(largePageSize - 1);
                mem = VirtualAlloc(NULL, allocSize, MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE);

                // Privilege no longer needed, restore previous state
                AdjustTokenPrivileges(hProcessToken, FALSE, &prevTp, 0, NULL, NULL);
            }
        }

        CloseHandle(hProcessToken);

        return mem;

#endif
    }

    inline void* aligned_large_pages_alloc(size_t allocSize)
    {

        // Try to allocate large pages
        void* mem = aligned_large_pages_alloc_windows(allocSize);

        // Fall back to regular, page aligned, allocation if necessary
        if (!mem)
            mem = VirtualAlloc(NULL, allocSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

        return mem;
    }

#else

    inline void* aligned_large_pages_alloc(size_t allocSize)
    {

#if defined(__linux__)
        constexpr size_t alignment = 2 * 1024 * 1024;   // assumed 2MB page size
#else
        constexpr size_t alignment = 4096;   // assumed small page size
#endif

        // round up to multiples of alignment
        size_t size = ((allocSize + alignment - 1) / alignment) * alignment;
        void* mem = std_aligned_alloc(alignment, size);
#if defined(MADV_HUGEPAGE)
        madvise(mem, size, MADV_HUGEPAGE);
#endif
        return mem;
    }

#endif

    /// aligned_large_pages_free() will free the previously allocated ttmem

#if defined(_WIN32)

    inline void aligned_large_pages_free(void* mem)
    {

        if (mem && !VirtualFree(mem, 0, MEM_RELEASE))
        {
            DWORD err = GetLastError();
            std::cerr << "Failed to free large page memory. Error code: 0x" << std::hex << err << std::dec << std::endl;
            exit(EXIT_FAILURE);
        }
    }

#else

    inline void aligned_large_pages_free(void* mem)
    {
        std_aligned_free(mem);
    }

#endif

    template<typename T>
    class ValueListInserter
    {
    public:
        ValueListInserter(T* v, std::size_t& s) : values(v), size(&s) {}

        void push_back(const T& value)
        {
            values[(*size)++] = value;
        }
    private:
        T* values;
        std::size_t* size;
    };

    template<typename T, std::size_t MaxSize>
    class ValueList
    {
    public:
        std::size_t size() const
        {
            return size_;
        }
        void resize(std::size_t newSize)
        {
            size_ = newSize;
        }
        void push_back(const T& value)
        {
            values_[size_++] = value;
        }
        T& operator[](std::size_t index)
        {
            return values_[index];
        }
        T* begin()
        {
            return values_;
        }
        T* end()
        {
            return values_ + size_;
        }
        const T& operator[](std::size_t index) const
        {
            return values_[index];
        }
        const T* begin() const
        {
            return values_;
        }
        const T* end() const
        {
            return values_ + size_;
        }
        operator ValueListInserter<T>()
        {
            return ValueListInserter(values_, size_);
        }

        void swap(ValueList& other)
        {
            const std::size_t maxSize = std::max(size_, other.size_);
            for (std::size_t i = 0; i < maxSize; ++i)
            {
                std::swap(values_[i], other.values_[i]);
            }
            std::swap(size_, other.size_);
        }

    private:
        T values_[MaxSize];
        std::size_t size_ = 0;
    };

    // IsLittleEndian : true if and only if the binary is compiled on a little endian machine
    static inline const union
    {
        uint32_t i;
        char c[4];
    } Le = {0x01020304};
    static inline const bool IsLittleEndian = (Le.c[0] == 4);

    // Version of the evaluation file
    constexpr std::uint32_t Version = 0x7AF32F20u;

    // Constant used in evaluation value calculation
    constexpr int OutputScale = 16;
    constexpr int WeightScaleBits = 6;

    // Size of cache line (in bytes)
    constexpr std::size_t CacheLineSize = 64;

// SIMD width (in bytes)
#if defined(USE_AVX2)
    constexpr std::size_t SimdWidth = 32;

#elif defined(USE_SSE2)
    constexpr std::size_t SimdWidth = 16;

#elif defined(USE_MMX)
    constexpr std::size_t SimdWidth = 8;

#elif defined(USE_NEON)
    constexpr std::size_t SimdWidth = 16;
#endif

    constexpr std::size_t MaxSimdWidth = 32;

    // Type of input feature after conversion
    using TransformedFeatureType = std::uint8_t;
    using IndexType = std::uint32_t;

    // Round n up to be a multiple of base
    template<typename IntType>
    constexpr IntType ceil_to_multiple(IntType n, IntType base)
    {
        return (n + base - 1) / base * base;
    }

    // read_little_endian() is our utility to read an integer (signed or unsigned, any size)
    // from a stream in little-endian order. We swap the byte order after the read if
    // necessary to return a result with the byte ordering of the compiling machine.
    template<typename IntType>
    inline IntType read_little_endian(std::istream& stream)
    {
        IntType result;

        if (IsLittleEndian)
            stream.read(reinterpret_cast<char*>(&result), sizeof(IntType));
        else
        {
            std::uint8_t u[sizeof(IntType)];
            typename std::make_unsigned<IntType>::type v = 0;

            stream.read(reinterpret_cast<char*>(u), sizeof(IntType));
            for (std::size_t i = 0; i < sizeof(IntType); ++i)
                v = (v << 8) | u[sizeof(IntType) - i - 1];

            std::memcpy(&result, &v, sizeof(IntType));
        }

        return result;
    }

    // write_little_endian() is our utility to write an integer (signed or unsigned, any size)
    // to a stream in little-endian order. We swap the byte order before the write if
    // necessary to always write in little endian order, independantly of the byte
    // ordering of the compiling machine.
    template<typename IntType>
    inline void write_little_endian(std::ostream& stream, IntType value)
    {

        if (IsLittleEndian)
            stream.write(reinterpret_cast<const char*>(&value), sizeof(IntType));
        else
        {
            std::uint8_t u[sizeof(IntType)];
            typename std::make_unsigned<IntType>::type v = value;

            std::size_t i = 0;
            // if constexpr to silence the warning about shift by 8
            if constexpr (sizeof(IntType) > 1)
            {
                for (; i + 1 < sizeof(IntType); ++i)
                {
                    u[i] = v;
                    v >>= 8;
                }
            }
            u[i] = v;

            stream.write(reinterpret_cast<char*>(u), sizeof(IntType));
        }
    }

    // read_little_endian(s, out, N) : read integers in bulk from a little indian stream.
    // This reads N integers from stream s and put them in array out.
    template<typename IntType>
    inline void read_little_endian(std::istream& stream, IntType* out, std::size_t count)
    {
        if (IsLittleEndian)
            stream.read(reinterpret_cast<char*>(out), sizeof(IntType) * count);
        else
            for (std::size_t i = 0; i < count; ++i)
                out[i] = read_little_endian<IntType>(stream);
    }

    // write_little_endian(s, values, N) : write integers in bulk to a little indian stream.
    // This takes N integers from array values and writes them on stream s.
    template<typename IntType>
    inline void write_little_endian(std::ostream& stream, const IntType* values, std::size_t count)
    {
        if (IsLittleEndian)
            stream.write(reinterpret_cast<const char*>(values), sizeof(IntType) * count);
        else
            for (std::size_t i = 0; i < count; ++i)
                write_little_endian<IntType>(stream, values[i]);
    }

}   // namespace Storm::NNUE
