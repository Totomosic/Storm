#pragma once
#include "Storm.h"
#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <functional>

namespace Storm
{

	class CommandManager
	{
	private:
		std::unordered_map<std::string, std::function<void(const std::vector<std::string>&)>> m_CommandMap;
		bool m_UsingNNUE;

		OpeningBook m_OpeningBook;

		StateInfo m_StateInfo;
		Position m_CurrentPosition;
		Search m_Search;
		SearchSettings m_Settings;
		std::atomic<bool> m_Searching;
		std::thread m_SearchThread;

		Move m_UndoMove;
		UndoInfo m_Undo;

	public:
		CommandManager();
		~CommandManager();

		void ExecuteCommand(const std::string& uciCommand);

	private:
		void Help();
		void Uci();
		void IsReady();
		void NewGame();
		void PrintBoard();
		void SetOption(std::string name, const std::string* value);
		void SetPositionFen(const std::string& fen);
		void ApplyMoves(const std::vector<std::string>& moves);
		void Eval();
		void Perft(int depth);
		void Go(const std::vector<std::string>& args);
		void Stop();
		void Quit();

		// Debug helpers
		void Moves();
		void Probe();
		void ProbeTT();

		// Utils
		std::unordered_set<Move> GetMoveList(const std::vector<std::string>& args, int offset) const;

	};

}
