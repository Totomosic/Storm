#pragma once
#include "EvalConstants.h"
#include "Move.h"
#include "ZobristHash.h"

#ifdef SWIG
#define STORM_API
#define ValueType int16_t
#endif

namespace Storm
{

    constexpr ValueType StaticEvalLimits = 4000;

    STORM_API enum EntryBound : uint8_t
    {
        BOUND_LOWER = 1 << 0,
        BOUND_UPPER = 1 << 1,
        BOUND_EXACT = BOUND_LOWER | BOUND_UPPER,
    };

    class STORM_API TranspositionTableEntry
    {
    private:
        ZobristHash m_Hash;
        Move m_Move;
        int8_t m_Depth;
        uint32_t m_ValueAndBound;

    public:
        inline ZobristHash GetHash() const
        {
            return m_Hash;
        }

        inline Move GetMove() const
        {
            return m_Move;
        }

        inline int GetDepth() const
        {
            return int(m_Depth);
        }

        // Mask out 3 most signficant bits
        inline ValueType GetValue() const
        {
            return int((m_ValueAndBound >> 13) & 0xFFFF) + VALUE_NONE;
        }

        inline ValueType GetStaticEvaluation() const
        {
            return int(m_ValueAndBound & 0x1FFF) - StaticEvalLimits;
        }

        inline EntryBound GetBound() const
        {
            return EntryBound((m_ValueAndBound >> 29) & 0x3);
        }

        inline void Update(
          ZobristHash hash, Move move, int depth, EntryBound bound, ValueType value, ValueType staticEval)
        {
            STORM_ASSERT(value >= VALUE_NONE, "Invalid Value");
            if (move != MOVE_NONE && m_Move == MOVE_NONE)
                m_Move = move;
            if ((bound == BOUND_EXACT && GetBound() != BOUND_EXACT) || hash != m_Hash || depth * 2 >= m_Depth)
            {
                m_Move = move;
                m_Hash = hash;
                m_Depth = depth;
                m_ValueAndBound =
                  (uint32_t(bound) << 29) | ((uint32_t(value - VALUE_NONE) & 0xFFFF) << 13) |
                  (uint32_t(std::clamp(staticEval, ValueType(-StaticEvalLimits), StaticEvalLimits) + StaticEvalLimits) &
                    0x1FFF);
            }
        }
    };

    class STORM_API TranspositionTable
    {
    public:
        static constexpr size_t DEFAULT_TABLE_SIZE = 32 * 1024 * 1024;

    private:
        std::unique_ptr<TranspositionTableEntry[]> m_Entries;
        size_t m_Mask;
        size_t m_EntryCount;

    public:
        TranspositionTable(size_t bytes = DEFAULT_TABLE_SIZE);

        bool IsSameSize(size_t bytes) const;

        inline TranspositionTableEntry* GetEntry(const ZobristHash& hash, bool& found) const
        {
            TranspositionTableEntry* entry = &m_Entries[HashToIndex(hash.Hash)];
            found = entry->GetHash() == hash;
            return entry;
        }

        inline size_t HashToIndex(uint64_t hash) const
        {
            // STORM_ASSERT((hash & m_Mask) < m_EntryCount, "Invalid index");
            STORM_ASSERT(((uint32_t)hash * (uint64_t)m_EntryCount) >> 32 < m_EntryCount, "Invalid Index");
            return ((uint32_t)hash * (uint64_t)m_EntryCount) >> 32;
            // return hash & m_Mask;
        }

        inline int HashFull() const
        {
            int used = 0;
            const int samples = 1000;
            for (int i = 0; i < samples; ++i)
            {
                if (m_Entries[i].GetMove() != MOVE_NONE)
                {
                    used++;
                }
            }
            return used / (samples / 1000);
        }

        void SetSize(size_t bytes);
        void Clear();

    private:
        size_t CalculateEntryCount(size_t bytes) const;
    };

}
