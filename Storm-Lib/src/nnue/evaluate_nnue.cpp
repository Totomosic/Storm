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

// Code for calculating NNUE evaluation function

#define EvalFileDefaultName "nn-9e3c6298299a.nnue"

#include <iostream>
#include <set>
#include <sstream>
#include <iomanip>
#include <fstream>

#include "../Position.h"
#include "../Types.h"
#include "../Evaluation.h"

#include "evaluate_nnue.h"

namespace Storm::NNUE
{

    // Input feature converter
    LargePagePtr<FeatureTransformer> featureTransformer;

    // Evaluation function
    AlignedPtr<Network> network[LayerStacks];

    // Evaluation function file name
    std::string fileName;
    std::string netDescription;

    namespace Detail
    {

        // Initialize the evaluation function parameters
        template<typename T>
        void initialize(AlignedPtr<T>& pointer)
        {

            pointer.reset(reinterpret_cast<T*>(std_aligned_alloc(alignof(T), sizeof(T))));
            std::memset(pointer.get(), 0, sizeof(T));
        }

        template<typename T>
        void initialize(LargePagePtr<T>& pointer)
        {

            static_assert(
              alignof(T) <= 4096, "aligned_large_pages_alloc() may fail for such a big alignment requirement of T");
            pointer.reset(reinterpret_cast<T*>(aligned_large_pages_alloc(sizeof(T))));
            std::memset(pointer.get(), 0, sizeof(T));
        }

        // Read evaluation function parameters
        template<typename T>
        bool read_parameters(std::istream& stream, T& reference)
        {

            std::uint32_t header;
            header = read_little_endian<std::uint32_t>(stream);
            if (!stream || header != T::get_hash_value())
                return false;
            return reference.read_parameters(stream);
        }

        // Write evaluation function parameters
        template<typename T>
        bool write_parameters(std::ostream& stream, const T& reference)
        {

            write_little_endian<std::uint32_t>(stream, T::get_hash_value());
            return reference.write_parameters(stream);
        }

    }   // namespace Detail

    // Initialize the evaluation function parameters
    void initialize()
    {

        Detail::initialize(featureTransformer);
        for (std::size_t i = 0; i < LayerStacks; ++i)
            Detail::initialize(network[i]);
    }

    // Read network header
    bool read_header(std::istream& stream, std::uint32_t* hashValue, std::string* desc)
    {
        std::uint32_t version, size;

        version = read_little_endian<std::uint32_t>(stream);
        *hashValue = read_little_endian<std::uint32_t>(stream);
        size = read_little_endian<std::uint32_t>(stream);
        if (!stream || version != Version)
            return false;
        desc->resize(size);
        stream.read(&(*desc)[0], size);
        return !stream.fail();
    }

    // Write network header
    bool write_header(std::ostream& stream, std::uint32_t hashValue, const std::string& desc)
    {
        write_little_endian<std::uint32_t>(stream, Version);
        write_little_endian<std::uint32_t>(stream, hashValue);
        write_little_endian<std::uint32_t>(stream, desc.size());
        stream.write(&desc[0], desc.size());
        return !stream.fail();
    }

    // Read network parameters
    bool read_parameters(std::istream& stream)
    {

        std::uint32_t hashValue;
        if (!read_header(stream, &hashValue, &netDescription))
            return false;
        if (hashValue != HashValue)
            return false;
        if (!Detail::read_parameters(stream, *featureTransformer))
            return false;
        for (std::size_t i = 0; i < LayerStacks; ++i)
            if (!Detail::read_parameters(stream, *(network[i])))
                return false;
        return stream && stream.peek() == std::ios::traits_type::eof();
    }

    // Write network parameters
    bool write_parameters(std::ostream& stream)
    {

        if (!write_header(stream, HashValue, netDescription))
            return false;
        if (!Detail::write_parameters(stream, *featureTransformer))
            return false;
        for (std::size_t i = 0; i < LayerStacks; ++i)
            if (!Detail::write_parameters(stream, *(network[i])))
                return false;
        return (bool)stream;
    }

    // Evaluation function. Perform differential calculation.
    ValueType EvaluateNNUE(const Position& pos, bool adjusted)
    {

        // We manually align the arrays on the stack because with gcc < 9.3
        // overaligning stack variables with alignas() doesn't work correctly.

        constexpr uint64_t alignment = CacheLineSize;
        int delta = 7;

#if defined(ALIGNAS_ON_STACK_VARIABLES_BROKEN)
        TransformedFeatureType
          transformedFeaturesUnaligned[FeatureTransformer::BufferSize + alignment / sizeof(TransformedFeatureType)];
        char bufferUnaligned[Network::BufferSize + alignment];

        auto* transformedFeatures = align_ptr_up<alignment>(&transformedFeaturesUnaligned[0]);
        auto* buffer = align_ptr_up<alignment>(&bufferUnaligned[0]);
#else
        alignas(alignment) TransformedFeatureType transformedFeatures[FeatureTransformer::BufferSize];
        alignas(alignment) char buffer[Network::BufferSize];
#endif

        const std::size_t bucket = (Popcount(pos.GetPieces()) - 1) / 4;
        const auto psqt = featureTransformer->transform(pos, transformedFeatures, bucket);
        const auto positional = network[bucket]->propagate(transformedFeatures, buffer)[0];

        // Give more value to positional evaluation when material is balanced
        ValueType eval;
        if (adjusted && abs(pos.GetNonPawnMaterial(COLOR_WHITE) - pos.GetNonPawnMaterial(COLOR_BLACK)) <=
                          RookValueMg - BishopValueMg)
            eval = static_cast<ValueType>(((128 - delta) * psqt + (128 + delta) * positional) / 128 / OutputScale);
        else
            eval = static_cast<ValueType>((psqt + positional) / OutputScale);
        return 136 * eval / 216;
    }

    struct NnueEvalTrace
    {
        static_assert(LayerStacks == PSQTBuckets);

        ValueType psqt[LayerStacks];
        ValueType positional[LayerStacks];
        std::size_t correctBucket;
    };

    static NnueEvalTrace trace_evaluate(const Position& pos)
    {

        // We manually align the arrays on the stack because with gcc < 9.3
        // overaligning stack variables with alignas() doesn't work correctly.

        constexpr uint64_t alignment = CacheLineSize;

#if defined(ALIGNAS_ON_STACK_VARIABLES_BROKEN)
        TransformedFeatureType
          transformedFeaturesUnaligned[FeatureTransformer::BufferSize + alignment / sizeof(TransformedFeatureType)];
        char bufferUnaligned[Network::BufferSize + alignment];

        auto* transformedFeatures = align_ptr_up<alignment>(&transformedFeaturesUnaligned[0]);
        auto* buffer = align_ptr_up<alignment>(&bufferUnaligned[0]);
#else
        alignas(alignment) TransformedFeatureType transformedFeatures[FeatureTransformer::BufferSize];
        alignas(alignment) char buffer[Network::BufferSize];
#endif

        NnueEvalTrace t {};
        t.correctBucket = (Popcount(pos.GetPieces()) - 1) / 4;
        for (std::size_t bucket = 0; bucket < LayerStacks; ++bucket)
        {
            const auto psqt = featureTransformer->transform(pos, transformedFeatures, bucket);
            const auto output = network[bucket]->propagate(transformedFeatures, buffer);

            int materialist = psqt;
            int positional = output[0];

            t.psqt[bucket] = static_cast<ValueType>(materialist / OutputScale);
            t.positional[bucket] = static_cast<ValueType>(positional / OutputScale);
        }

        return t;
    }

    // Load eval, from a file stream or a memory stream
    bool load_eval(std::string name, std::istream& stream)
    {

        initialize();
        fileName = name;
        return read_parameters(stream);
    }

}   // namespace Storm::NNUE
