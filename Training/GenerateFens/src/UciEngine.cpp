#include "UciEngine.h"

namespace Storm
{

	UciEngine::UciEngine(const std::string& command)
		: m_Process(command)
	{
		ReadEngineStrings();
	}

	void UciEngine::SendCommand(const std::string& command)
	{
		std::string cmd = command + "\n";
		m_Process.WriteStdin(cmd.data(), cmd.size());
	}

	PositionResult UciEngine::AnalyzePosition(const std::string& fen, int depth, bool verbose)
	{
		SendCommand("isready");
		ReadEngineStrings();
		SendCommand("position fen " + fen);
		SendCommand("go depth " + std::to_string(depth));

		std::string token = "bestmove ";
		std::string cpToken = "score cp ";
		std::string mateToken = "score mate ";

		int currentScore = 0;
		int currentMate = 0;

		while (true)
		{
			for (const std::string& line : ReadEngineStrings())
			{
				if (verbose)
					std::cout << line << std::endl;
				size_t cp = line.find(cpToken);
				if (cp != std::string::npos)
				{
					size_t cpStart = cp + cpToken.size();
					currentScore = std::stoi(line.substr(cpStart, line.find_first_of(' ', cpStart) - cpStart));
				}
				size_t mate = line.find(mateToken);
				if (mate != std::string::npos)
				{
					size_t mateStart = mate + mateToken.size();
					currentMate = std::stoi(line.substr(mateStart, line.find_first_of(' ', mateStart) - mateStart));
				}
				if (line.find(token) != std::string::npos)
				{
					size_t start = line.find_first_of(' ') + 1;
					size_t end = line.find_first_of(' ', start);
					std::string bestmove = line.substr(start, end - start);

					return { currentScore, currentMate, bestmove };
				}
			}
		}
		return {};
	}

	std::vector<std::string> UciEngine::ReadEngineStrings()
	{
		char buffer[8192];
		size_t read = m_Process.ReadStdout(buffer, sizeof(buffer) - 1);
		if (read == 0)
			return {};
		buffer[read] = '\0';
		return Split(std::string(buffer, read), "\n");
	}

}
