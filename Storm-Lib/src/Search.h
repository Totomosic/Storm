#pragma once
#include "Position.h"
#include "MoveSelection.h"
#include "Format.h"
#include "Evaluation.h"
#include "TranspositionTable.h"
#include "SearchConstants.h"
#include "SearchData.h"
#include "TimeManager.h"
#include "Book.h"

#include <atomic>
#include <chrono>

namespace Storm
{

	void InitSearch();

	inline std::string FormatPV(const RootMove& move, ValueType alpha, ValueType beta, int depth, int multiPv, size_t nodes, size_t elapsedMilliseconds, bool isBookmove)
	{
		std::stringstream ss;
		ss << "info depth " << depth << " seldepth " << move.SelDepth << " score ";
		if (!IsMateScore(move.Score))
			ss << "cp " << move.Score;
		else
		{
			if (move.Score > 0)
				ss << "mate " << (GetPliesFromMateScore(move.Score) / 2 + 1);
			else
				ss << "mate " << -(GetPliesFromMateScore(move.Score) / 2);
		}
		if (move.Score <= alpha)
			ss << " upperbound";
		else if (move.Score >= beta)
			ss << " lowerbound";
		ss << " nodes " << nodes;
		ss << " nps " << (size_t)(nodes * 1000 / (elapsedMilliseconds + 1));
		ss << " time " << elapsedMilliseconds;
		ss << " multipv " << (multiPv + 1);
		if (isBookmove)
			std::cout << " bookmove";
		//if (hashFull >= 500)
		//    std::cout << " hashfull " << hashFull;
		ss << " pv";
		for (Move move : move.Pv)
		{
			ss << " " << UCI::FormatMove(move);
		}
		return ss.str();
	}

	struct STORM_API BestMove
	{
	public:
		Storm::Move Move;
		Storm::Move Ponder = MOVE_NONE;
	};

	struct STORM_API SearchSettings
	{
	public:
		int MultiPv = 1;
		int SkillLevel = 20;
	};

	struct STORM_API SearchResult
	{
	public:
		std::vector<Move> PV;
		ValueType Score;
		Move BestMove;
		int PVIndex;
		int Depth;
		int SelDepth;
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
		SearchSettings m_Settings;
		TimeManager m_TimeManager;
		const OpeningBook* m_Book;

		bool m_Log;
		SearchLimits m_Limits;
		std::vector<RootMove> m_RootMoves;
		size_t m_Nodes;
		int m_PvIndex;

		bool m_Stopped;
		std::atomic<bool> m_ShouldStop;

		std::unique_ptr<SearchTables> m_SearchTables;

	public:
		Search(size_t ttSize, bool log = true);

		inline const SearchSettings& GetSettings() const { return m_Settings; }
		inline const TranspositionTable& GetTranspositionTable() const { return m_TranspositionTable; }
		void SetSettings(const SearchSettings& settings);
		void PushPosition(const ZobristHash& hash);
		inline void SetOpeningBook(const OpeningBook* book) { m_Book = book; }

		size_t Perft(const Position& position, int depth);
		void Ponder(const Position& position, SearchLimits limits, const std::function<void(const SearchResult&)>& callback = {});
		void Ponder(const Position& position, const std::function<void(const SearchResult&)>& callback = {});
		BestMove SearchBestMove(const Position& position, SearchLimits limits);
		BestMove SearchBestMove(const Position& position, SearchLimits limits, const std::function<void(const SearchResult&)>& callback);

		void Stop();

	private:
		size_t PerftPosition(Position& position, int depth);

		RootMove SearchRoot(Position& position, int depth, const std::function<void(const SearchResult&)>& callback);
		template<NodeType NT>
		ValueType SearchPosition(Position& position, SearchStack* stack, int depth, ValueType alpha, ValueType beta, int& selDepth, bool cutNode);
		template<NodeType NT>
		ValueType QuiescenceSearch(Position& position, SearchStack* stack, int depth, ValueType alpha, ValueType beta, bool cutNode);

		void UpdateQuietStats(SearchStack* stack, Move move);

		bool IsDraw(const Position& position, SearchStack* stack) const;

		constexpr ValueType GetValueForTT(ValueType value, int ply) const {
			return IsMateScore(value) ? (value < 0 ? value - ply : value + ply) : value;
		}

		constexpr ValueType GetValueFromTT(ValueType value, int ply) const
		{
			return IsMateScore(value) ? (value < 0 ? value + ply : value - ply) : value;
		}

		bool CheckLimits() const;
		std::vector<RootMove> GenerateRootMoves(const Position& position, const std::unordered_set<Move>& only) const;
		int SelectBestMoveIndex(int multipv, int skillLevel) const;
		void SetTimeManagementFromLimits(const Position& position, const SearchLimits& limits);

		SearchStack* InitStack(SearchStack* stack, int count, const Position& position, Move* pv, ZobristHash* history) const;

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
