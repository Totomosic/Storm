#pragma once
#include "Storm.h"
#include "UciEngine.h"
#include "ThreadSafeFileWriter.h"

#include <unordered_set>
#include <random>

namespace Storm
{

	struct STORM_API SharedData
	{
	public:
		const int64_t TotalIterations;
		int64_t CompletedIterations;
		std::mutex Mutex;
	};

	class STORM_API Searcher
	{
	private:
		struct STORM_API SearchResult
		{
		public:
			bool Continue;
		};

	private:
		UciEngine m_Engine;
		ThreadSafeFileWriter& m_Writer;
		SharedData& m_SharedData;
		int64_t m_Iterations;
		int64_t m_CompletedIterations;
		int m_Depth;
		mutable std::mt19937 m_Random;
		float m_BestMoveChance;

		std::unordered_set<std::string> m_WrittenFens;
		std::vector<std::string> m_StringCache;

		int m_EvalLimit;

	public:
		Searcher(const std::string& engineCommand, ThreadSafeFileWriter& writer, SharedData& sharedData, int64_t iterations, int depth, size_t seed);

		inline void SetBestMoveChance(float chance) { m_BestMoveChance = chance; }
		void Start(const Position& initialPosition);

	private:
		SearchResult SearchPosition(Position& position, int ply);
		float GetRandom() const;
		Move PickRandom(std::vector<Move>& moves) const;
		std::string GetComparableFen(const std::string& fen) const;

		void WriteData(const std::string& fen, const PositionResult& result);
		void FlushData();

	};

}
