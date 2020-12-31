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
		int MoveCount;
		Move SkipMove = MOVE_NONE;
		Storm::Position Position;
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

	constexpr int P_LIMIT = 16;
	using CounterMoveTable = Move[SQUARE_MAX][SQUARE_MAX];
	using HistoryTable = int16_t[COLOR_MAX][SQUARE_MAX][SQUARE_MAX];
	using CounterMoveHistoryTable = int16_t[P_LIMIT][SQUARE_MAX][P_LIMIT * SQUARE_MAX];

	constexpr int MaxCmhPly = 2;

	struct STORM_API SearchTables
	{
	public:
		CounterMoveTable CounterMoves;
		HistoryTable History;
		CounterMoveHistoryTable CounterMoveHistory;
	};

	inline void SetCounterMoveHistoryPointer(int16_t** cmhPtr, SearchStack* stack, CounterMoveHistoryTable* cmhTable, int ply)
	{
		for (int i = 0; i < MaxCmhPly; i++)
		{
			SearchStack* ss = stack - i;
			if (ply > i && ss->CurrentMove != MOVE_NONE)
			{
				SquareIndex to = GetToSquare(ss->CurrentMove);
				cmhPtr[i] = (*cmhTable)[ss->Position.GetPieceOnSquare(to)][to];
			}
			else
				cmhPtr[i] = nullptr;
		}
	}

	inline ValueType GetHistoryScore(SearchStack* stack, const Position& position, int16_t** cmhPtr, Move move, const SearchTables* tables)
	{
		SquareIndex from = GetFromSquare(move);
		SquareIndex to = GetToSquare(move);

		ValueType score = tables->History[position.ColorToMove][from][to];
		if (cmhPtr)
		{
			int piecePosition = position.GetPieceOnSquare(from) * SQUARE_MAX + to;
			for (int i = 0; i < MaxCmhPly; i++)
			{
				if (cmhPtr[i] != nullptr)
					score += cmhPtr[i][piecePosition];
			}
		}
		return score;
	}

	inline void AddToHistory(SearchStack* stack, int16_t** cmhPtr, SearchTables* tables, Move move, ValueType score)
	{
		SquareIndex from = GetFromSquare(move);
		SquareIndex to = GetToSquare(move);
		int piecePosition = stack->Position.GetPieceOnSquare(from) * SQUARE_MAX + to;

		int16_t* item = &tables->History[stack->Position.ColorToMove][from][to];
		*item += score - (*item) * std::abs(score) / MAX_HISTORY_SCORE;

		for (int i = 0; i < MaxCmhPly; i++)
		{
			if (cmhPtr[i] != nullptr)
			{
				item = &cmhPtr[i][piecePosition];
				*item += score = (*item) * std::abs(score) / MAX_HISTORY_SCORE;
			}
		}
	}

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

		std::unique_ptr<SearchTables> m_SearchTables;

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

		void UpdateQuietStats(SearchStack* stack, Move move);

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

		template<typename T, size_t Width, size_t Height, size_t Depth>
		void ClearTable(T table[Width][Height][Depth], T value)
		{
			for (size_t i = 0; i < Width; i++)
			{
				for (size_t j = 0; j < Height; j++)
				{
					for (size_t k = 0; k < Depth; k++)
						table[i][j][k] = value;
				}
			}
		}

	};

}
