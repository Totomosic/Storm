#pragma once
#include <functional>

#include "EvalConstants.h"
#include "Evaluation.h"
#include "Format.h"
#include "MoveSelection.h"
#include "SearchConstants.h"
#include "SearchData.h"
#include "TimeManager.h"
#include "TranspositionTable.h"

#ifdef SWIG
#define STORM_API
#define ValueType int16_t
#endif

namespace Storm
{

	constexpr ValueType GetValueForTT(ValueType value, int ply)
	{
		return IsMateScore(value) ? (value < 0 ? value - ply : value + ply) : value;
	}

	constexpr ValueType GetValueFromTT(ValueType value, int ply)
	{
		return IsMateScore(value) ? (value < 0 ? value + ply : value - ply) : value;
	}

	struct STORM_API SearchSettings
	{
	public:
		int MultiPv = 1;
		int SkillLevel = 20;
		int Threads = 1;
		size_t HashBytes = 0;
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

	struct STORM_API BestMove
	{
	public:
		Storm::Move Move;
		Storm::Move PonderMove = MOVE_NONE;
	};

	class STORM_API Search
	{
	public:
		using SearchCallback = std::function<void(const SearchResult&)>;

	private:
		struct STORM_API Thread
		{
		public:
			static constexpr int StackOffset = 4;
		public:
			bool Initialized = false;
			SearchStack Stack[MAX_PLY + StackOffset + 1];
			Move PvBuffer[MAX_PLY + 1];
			std::unique_ptr<ZobristHash[]> PositionHistory = nullptr;
			size_t Nodes = 0;
			int Depth = 0;
			int SelDepth = 0;
			int PvIndex = 0;
			int BestMoveChanges = 0;
			int ThreadIndex = 0;
			std::vector<RootMove> RootMoves;
			SearchTables Tables;
			StateInfo RootStateInfo;
			Storm::Position Position;

		public:
			inline bool IsMain() const { return ThreadIndex == 0; }
		};

	private:
		TimeManager m_TimeManager;
		TranspositionTable m_TranspositionTable;
		SearchLimits m_Limits;
		SearchSettings m_Settings;
		std::vector<ZobristHash> m_PositionHistory;
		std::atomic_bool m_ShouldStop;
		bool m_Stopped;
		bool m_EnableLogging;
		std::vector<Thread> m_Threads;

	public:
		Search();

		inline TranspositionTable& GetTranspositionTable() { return m_TranspositionTable; }
		inline const SearchSettings& GetSettings() const { return m_Settings; }

		inline void SetLogging(bool enabled) { m_EnableLogging = enabled; }
		inline void SetSettings(const SearchSettings& settings) { m_Settings = settings; }
		void PushPosition(const ZobristHash& hash);
		void Ponder(const Position& position, SearchLimits limits, const SearchCallback& callback = {});
		BestMove SearchBestMove(const Position& position, SearchLimits limits, const SearchCallback& callback = {});
		size_t Perft(const Position& position, int depth);
		inline void Reset()
		{
			m_TranspositionTable.Clear();
		}

		void Stop();

	private:
		size_t PerftPosition(Position& position, int depth);
		BestMove BeginSearch(Position& position, int depth, const SearchCallback& callback);
		void IterativeDeepeningSearch(Thread* thread, int maxDepth, const SearchCallback& callback);
		ValueType AspirationWindowSearch(Thread* thread, SearchStack* stack, const SearchCallback& callback);
		template<bool IsPvNode>
		ValueType AlphaBetaSearch(Thread* thread, SearchStack* stack, int depth, ValueType alpha, ValueType beta, bool cutNode);
		template<bool IsPvNode>
		ValueType QuiescenceSearch(Thread* thread, SearchStack* stack, int depth, ValueType alpha, ValueType beta, bool cutNode);

		void UpdateQuietStats(Thread* thread, SearchStack* stack, Move move) const;
		bool IsDraw(const Position& position, SearchStack* stack) const;
		bool ShouldStopSearch(Thread* thread) const;
		std::vector<RootMove> GenerateRootMoves(const Position& position, const std::unordered_set<Move>& only) const;
		int SelectBestMoveIndex(Thread* thread, int multipv, int skillLevel) const;
		void InitTimeManagement(const Position& position);
		void CreateAndInitializeThreads(Position& position, int count);
		void InitializeThread(Position& position, Thread* thread, int index);

		inline size_t GetTotalNodes() const
		{
			size_t count = 0;
			for (const Thread& thread : m_Threads)
			{
				count += thread.Nodes;
			}
			return count;
		}
	};

	void InitSearch();
}
