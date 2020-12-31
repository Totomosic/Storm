#pragma once
#include "Types.h"
#include "Position.h"
#include "Move.h"

#include <vector>

namespace Storm
{

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

	constexpr int16_t MAX_HISTORY_SCORE = 1 << 13;

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
			if (ply > i && (ss - 1)->CurrentMove != MOVE_NONE)
			{
				SquareIndex to = GetToSquare((ss - 1)->CurrentMove);
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

	inline void AddToHistory(const Position& position, SearchStack* stack, int16_t** cmhPtr, SearchTables* tables, Move move, ValueType score)
	{
		SquareIndex from = GetFromSquare(move);
		SquareIndex to = GetToSquare(move);
		int piecePosition = position.GetPieceOnSquare(from) * SQUARE_MAX + to;

		int16_t* item = &tables->History[position.ColorToMove][from][to];
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

	struct STORM_API SearchLimits
	{
	public:
		bool Infinite = false;
		int Depth = -1;
		int Milliseconds = -1;
		int Nodes = -1;
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

}
