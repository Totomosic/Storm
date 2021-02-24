#pragma once
#include "Types.h"
#include <chrono>
#include <iostream>

namespace Storm
{

	constexpr size_t MoveOverheadBufferMS = 100;

	class STORM_API TimeManager
	{
	private:
		int m_Milliseconds = -1;

		std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime;
		size_t m_AllocatedTime;

	public:
		TimeManager();

		inline void SetMillisecondsToMove(size_t milliseconds) { m_Milliseconds = int(milliseconds); }

		void StartSearch();
		inline bool IsSearchComplete() const
		{
			return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - m_StartTime).count() >= m_AllocatedTime - MoveOverheadBufferMS;
		}
	};

}
