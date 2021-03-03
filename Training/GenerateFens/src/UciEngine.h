#pragma once
#include "Storm.h"
#include "Subprocess.h"

namespace Storm
{

	struct STORM_API PositionResult
	{
	public:
		int Score;
		int Mate;
		std::string BestMove;
	};

	class STORM_API UciEngine
	{
	private:
		Subprocess m_Process;

	public:
		UciEngine(const std::string& command);

		void SendCommand(const std::string& command);
		PositionResult AnalyzePosition(const std::string& fen, int depth, bool verbose);

	private:
		std::vector<std::string> ReadEngineStrings();

	};

}
