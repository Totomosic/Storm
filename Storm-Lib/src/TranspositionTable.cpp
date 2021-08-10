#include "TranspositionTable.h"

namespace Storm
{

	TranspositionTable::TranspositionTable(size_t bytes)
		: m_Entries(), m_EntryCount(0), m_Mask(0)
	{
		SetSize(bytes);
	}

	bool TranspositionTable::IsSameSize(size_t bytes) const
	{
		return CalculateEntryCount(bytes) == m_EntryCount;
	}

	void TranspositionTable::SetSize(size_t bytes)
	{
		size_t nEntries = CalculateEntryCount(bytes);
		if (nEntries > 0)
		{
			m_Mask = nEntries - 1;
			m_EntryCount = m_Mask + 1;
			m_Entries = std::make_unique<TranspositionTableEntry[]>(m_EntryCount);
			Clear();
		}
	}

	void TranspositionTable::Clear()
	{
		std::memset(m_Entries.get(), 0, m_EntryCount * sizeof(TranspositionTableEntry));
	}

	size_t TranspositionTable::CalculateEntryCount(size_t bytes) const
	{
		size_t nEntries = bytes / sizeof(TranspositionTableEntry);
		if (nEntries > 0)
		{
			uint8_t nBits = 1;
			size_t ret = 1;
			while (nEntries >>= 1)
			{
				nBits++;
				ret <<= 1;
			}
			return ret;
		}
		return 0;
	}

}
