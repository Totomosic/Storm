#pragma once
#include "Types.h"
#include <chrono>
#include <iostream>

namespace Storm
{

	class STORM_API TimeManager
	{
	private:
		size_t m_TimeRemainingMS;
		size_t m_IncrementMS;

		// Overrides time management algorithm
		int m_Milliseconds = -1;

		std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime;
		size_t m_AllocatedTime;

	public:
		TimeManager();

		inline bool Enabled() const { return m_Milliseconds <= 0; }
		inline void Disable() { m_Milliseconds = -1; }
		inline void SetRemainingTime(size_t milliseconds) { m_TimeRemainingMS = milliseconds; }
		inline void SetIncrement(size_t milliseconds) { m_IncrementMS = milliseconds; }
		inline void SetMillisecondsToMove(size_t milliseconds) { m_Milliseconds = int(milliseconds); }

		void StartSearch();
		inline bool IsSearchComplete() const
		{
			return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - m_StartTime).count() >= m_AllocatedTime;
		}
	};

}
