#pragma once
#include "EvalConstants.h"
#include "Move.h"
#include "ZobristHash.h"

namespace Storm
{

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
		inline ZobristHash GetHash() const { return m_Hash; }
		inline Move GetMove() const { return m_Move; }
		inline int GetDepth() const { return int(m_Depth); }
		// Mask out 3 most signficant bits
		inline ValueType GetValue() const { return int(m_ValueAndBound & 0x1FFFFFFF) + VALUE_NONE; }
		inline EntryBound GetBound() const { return EntryBound(m_ValueAndBound >> 29); }

		inline void Update(ZobristHash hash, Move move, int depth, EntryBound bound, ValueType value)
		{
			STORM_ASSERT(value >= VALUE_NONE, "Invalid Value");
			if (move != MOVE_NONE || hash != m_Hash)
				m_Move = move;
			if (bound == BOUND_EXACT || hash != m_Hash || depth > m_Depth - 4)
			{
				m_Hash = hash;
				m_Depth = depth;
				m_ValueAndBound = (uint32_t(bound) << 29) | (uint32_t(value - VALUE_NONE) & 0x1FFFFFFF);
			}
		}
	};

	class STORM_API TranspositionTable
	{
	public:
		static constexpr size_t DEFAULT_TABLE_SIZE = 50 * 1024 * 1024;

	private:
		std::unique_ptr<TranspositionTableEntry[]> m_Entries;
		size_t m_Mask;
		size_t m_EntryCount;

	public:
		TranspositionTable(size_t bytes = DEFAULT_TABLE_SIZE);

		inline TranspositionTableEntry* GetEntry(const ZobristHash& hash, bool& found) const
		{
			TranspositionTableEntry* entry = &m_Entries[HashToIndex(hash.Hash)];
			found = entry->GetHash() == hash;
			return entry;
		}

		inline size_t HashToIndex(uint64_t hash) const
		{
			STORM_ASSERT((hash & m_Mask) < m_EntryCount, "Invalid index");
			return hash & m_Mask;
		}

	};

}
