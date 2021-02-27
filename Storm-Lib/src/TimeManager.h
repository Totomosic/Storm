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
		std::chrono::time_point<std::chrono::high_resolution_clock> m_DepthStartTime;
		size_t m_AllocatedTime;

	public:
		TimeManager();

		inline size_t TotalElapsedMs() const { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - m_StartTime).count(); }
		inline size_t ElapsedMs() const { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - m_DepthStartTime).count(); }
		inline void SetMillisecondsToMove(size_t milliseconds) { m_Milliseconds = int(milliseconds); }

		void StartSearch();
		void StartNewDepth();
		inline bool IsSearchComplete() const
		{
			return TotalElapsedMs() >= m_AllocatedTime - MoveOverheadBufferMS;
		}
	};

}
