#pragma once
#include "Position.h"
#include "MoveSelection.h"
#include "Format.h"
#include "Evaluation.h"
#include "TranspositionTable.h"
#include "SearchConstants.h"

#include <atomic>
#include <chrono>

namespace Storm
{

	void InitSearch();

	struct STORM_API SearchLimits
	{
	public:
		bool Infinite = false;
		int Depth = -1;
		int Milliseconds = -1;
		int Nodes = -1;
	};

	struct STORM_API SearchStack
	{
	public:
		int Ply;
		Move* PV;
		Move Killers[2];
		Move CurrentMove;
		ValueType StaticEvaluation;
		ZobristHash* PositionHistory;
	};

	class STORM_API RootMove
	{
	public:
		std::vector<Move> Pv;
		ValueType Score;
		int SelDepth;

	public:
		inline bool operator==(Move move) const
		{
			return move == Pv[0];
		}

		inline friend bool operator<(const RootMove& left, const RootMove& right)
		{
			return right.Score < left.Score;
		}
	};

	class STORM_API Search
	{
	public:
		enum NodeType
		{
			PV,
			NonPV,
		};

	private:
		TranspositionTable m_TranspositionTable;
		std::vector<ZobristHash> m_PositionHistory;

		bool m_Log;
		SearchLimits m_Limits;
		std::vector<RootMove> m_RootMoves;
		size_t m_Nodes;

		std::chrono::time_point<std::chrono::high_resolution_clock> m_StartRootTime;
		std::chrono::time_point<std::chrono::high_resolution_clock> m_StartSearchTime;
		bool m_Stopped;
		std::atomic<bool> m_ShouldStop;

		Move m_CounterMoves[SQUARE_MAX][SQUARE_MAX];

	public:
		Search(size_t ttSize, bool log = true);

		void PushPosition(const ZobristHash& hash);

		size_t Perft(const Position& position, int depth);
		void Ponder(const Position& position, SearchLimits limits = {});
		Move SearchBestMove(const Position& position, SearchLimits limits);

		void Stop();

	private:
		size_t PerftPosition(Position& position, int depth);

		RootMove SearchRoot(Position& position, int depth);
		template<NodeType NT>
		ValueType SearchPosition(Position& position, SearchStack* stack, int depth, ValueType alpha, ValueType beta, int& selDepth, bool cutNode);
		template<NodeType NT>
		ValueType QuiescenceSearch(Position& position, SearchStack* stack, int depth, ValueType alpha, ValueType beta, bool cutNode);

		bool IsDraw(const Position& position, SearchStack* stack) const;
		constexpr ValueType MateIn(int ply) const { return VALUE_MATE - ply; }
		constexpr ValueType MatedIn(int ply) const { return -VALUE_MATE + ply; }

		constexpr ValueType GetValueForTT(ValueType value, int ply) const {
			return IsMateScore(value) ? (value < 0 ? value - ply : value + ply) : value;
		}

		constexpr ValueType GetValueFromTT(ValueType value, int ply) const
		{
			return IsMateScore(value) ? (value < 0 ? value + ply : value - ply) : value;
		}

		constexpr int GetPliesFromMateScore(ValueType score) const { return score < 0 ? score + VALUE_MATE : VALUE_MATE - score; }
		constexpr bool IsMateScore(ValueType score) const { return score >= MateIn(MAX_PLY) || score <= MatedIn(MAX_PLY); }
		bool CheckLimits() const;
		std::vector<RootMove> GenerateRootMoves(const Position& position) const;

		template<typename T, size_t Width, size_t Height>
		void ClearTable(T table[Width][Height], T value)
		{
			for (size_t i = 0; i < Width; i++)
			{
				for (size_t j = 0; j < Height; j++)
					table[i][j] = value;
			}
		}

	};

}
