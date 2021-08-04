#include "TranspositionTable.h"

namespace Storm
{

	TranspositionTable::TranspositionTable(size_t bytes)
		: m_Entries(), m_EntryCount(0), m_Mask(0)
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
			m_Mask = ret - 1;
			m_EntryCount = m_Mask + 1;
			m_Entries = std::make_unique<TranspositionTableEntry[]>(m_EntryCount);
			std::memset(m_Entries.get(), 0, m_EntryCount * sizeof(TranspositionTableEntry));
		}
	}

}
