#pragma once
#include "Types.h"
#include <chrono>
#include <iostream>

#ifdef SWIG
#define STORM_API
#define ValueType int16_t
#endif

namespace Storm
{

	constexpr size_t MoveOverheadBufferMS = 100;

	class STORM_API TimeManager
	{
	private:
		int64_t m_Milliseconds = -1;

		std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime;
		std::chrono::time_point<std::chrono::high_resolution_clock> m_DepthStartTime;
		size_t m_AllocatedTime;
		size_t m_MaxTime;
		float m_AllocatedTimeMultiplier;

	public:
		TimeManager();

		inline size_t TotalElapsedMs() const { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - m_StartTime).count(); }
		inline size_t ElapsedMs() const { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - m_DepthStartTime).count(); }
		inline void SetOptimalTime(size_t milliseconds) { m_Milliseconds = int64_t(milliseconds); }
		inline void SetMaxTime(size_t milliseconds) { m_MaxTime = milliseconds; }
		inline void SetAllocatedTimeMultiplier(float multiplier) { m_AllocatedTimeMultiplier = multiplier; }
		inline size_t GetMaxTime() const { return m_MaxTime; }

		void StartSearch();
		void StartNewDepth();
		inline bool IsSearchComplete() const
		{
			return TotalElapsedMs() >= GetAdjustedAllocatedTime() - MoveOverheadBufferMS;
		}

		inline int64_t RemainingAllocatedTime() const { return GetAdjustedAllocatedTime() - TotalElapsedMs() - MoveOverheadBufferMS; }

	private:
		inline size_t GetAdjustedAllocatedTime() const { return std::clamp(size_t(m_AllocatedTime * m_AllocatedTimeMultiplier), size_t(0ULL), m_MaxTime); }
	};

}
