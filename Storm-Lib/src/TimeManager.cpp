#include "TimeManager.h"

namespace Storm
{

	TimeManager::TimeManager()
		: m_Milliseconds(-1), m_StartTime(), m_AllocatedTime(0), m_AllocatedTimeMultiplier(1.0f)
	{
	}

	void TimeManager::StartSearch()
	{
		m_StartTime = std::chrono::high_resolution_clock::now();
		m_AllocatedTime = size_t(m_Milliseconds);
	}

	void TimeManager::StartNewDepth()
	{
		m_DepthStartTime = std::chrono::high_resolution_clock::now();
	}

}
