#include "TimeManager.h"

namespace Storm
{

	TimeManager::TimeManager()
		: m_TimeRemainingMS(0), m_IncrementMS(0), m_Milliseconds(-1), m_StartTime(), m_AllocatedTime(0)
	{
	}

	void TimeManager::StartSearch()
	{
		m_StartTime = std::chrono::high_resolution_clock::now();
		if (!Enabled())
			m_AllocatedTime = size_t(m_Milliseconds);
		else
		{

		}
	}

}
