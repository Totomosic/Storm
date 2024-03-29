#include "CommandManager.h"

namespace Storm
{

    CommandManager::CommandManager()
        : m_CommandMap(),
          m_UsingNNUE(IsNNUEAvailable()),
          m_OpeningBook(),
          m_CurrentPosition(CreateStartingPosition()),
          m_Search(),
          m_Searching(false),
          m_SearchThread(),
          m_UndoMove(MOVE_NONE),
          m_Undo()
    {
        m_CurrentPosition.SetNetworkEnabled(m_UsingNNUE);
        m_CommandMap["help"] = [this](const std::vector<std::string>& args) { Help(); };

        m_CommandMap["isready"] = [this](const std::vector<std::string>& args) { IsReady(); };

        m_CommandMap["uci"] = [this](const std::vector<std::string>& args) { Uci(); };

        m_CommandMap["ucinewgame"] = [this](const std::vector<std::string>& args) { NewGame(); };

        m_CommandMap["setoption"] = [this](const std::vector<std::string>& args)
        {
            if (args.size() > 1)
            {
                auto it = args.begin();
                while (it != args.end() && *it != "name")
                    it++;
                if (it != args.end())
                    it++;
                if (it != args.end())
                {
                    std::string name = *it;
                    it++;
                    while (it != args.end() && *it != "value")
                    {
                        name += " " + *it;
                        it++;
                    }
                    if (it != args.end())
                        it++;
                    if (it != args.end())
                    {
                        std::string value = *it;
                        it++;
                        while (it != args.end())
                        {
                            value += " " + *it;
                            it++;
                        }
                        SetOption(name, &value);
                    }
                    else
                    {
                        SetOption(name, nullptr);
                    }
                }
            }
        };

        m_CommandMap["d"] = [this](const std::vector<std::string>& args) { PrintBoard(); };

        m_CommandMap["position"] = [this](const std::vector<std::string>& args)
        {
            if (args.size() > 0)
            {
                auto it = args.begin();
                if (*it == "fen")
                {
                    it++;
                    std::string fen = *it++;
                    while (it != args.end() && *it != "moves")
                    {
                        fen += " " + *it++;
                    }
                    SetPositionFen(fen);
                }
                if (it != args.end() && *it == "startpos")
                {
                    SetPositionFen(GetFENFromPosition(CreateStartingPosition()));
                    it++;
                }
                if (it != args.end() && *it == "moves")
                {
                    ApplyMoves({it + 1, args.end()});
                }
            }
        };

        m_CommandMap["eval"] = [this](const std::vector<std::string>& args) { Eval(); };

        m_CommandMap["perft"] = [this](const std::vector<std::string>& args)
        {
            if (args.size() > 0)
            {
                int depth = std::stoi(args[0]);
                Perft(depth);
            }
        };

        m_CommandMap["go"] = [this](const std::vector<std::string>& args) { Go(args); };

        m_CommandMap["moves"] = [this](const std::vector<std::string>& args) { Moves(); };

        m_CommandMap["probe"] = [this](const std::vector<std::string>& args) { Probe(); };

        m_CommandMap["probett"] = [this](const std::vector<std::string>& args) { ProbeTT(); };

        m_CommandMap["stop"] = [this](const std::vector<std::string>& args) { Stop(); };

        m_CommandMap["quit"] = [this](const std::vector<std::string>& args) { Quit(); };

        m_CommandMap["undo"] = [this](const std::vector<std::string>& args)
        {
            if (m_UndoMove != MOVE_NONE && !m_Searching)
            {
                m_CurrentPosition.UndoMove(m_UndoMove, m_Undo);
                m_UndoMove = MOVE_NONE;
            }
            else
                std::cout << "Cannot Undo" << std::endl;
        };

        m_CommandMap["nnue"] = [this](const std::vector<std::string>& args)
        {
            if (m_UsingNNUE)
            {
                std::cout << "Using NNUE Evaluation :- " << GetNNUEFilename() << std::endl;
            }
            else
            {
                std::cout << "Using classical evaluation." << std::endl;
            }
        };

        ExecuteCommand("ucinewgame");
    }

    CommandManager::~CommandManager()
    {
        if (m_SearchThread.joinable())
            m_SearchThread.join();
    }

    void CommandManager::ExecuteCommand(const std::string& uciCommand)
    {
        std::vector<std::string> commandParts = Split(uciCommand, " ");
        if (commandParts.size() > 0)
        {
            std::string& command = commandParts[0];
            if (m_CommandMap.find(command) != m_CommandMap.end())
            {
                std::vector<std::string> args = {commandParts.begin() + 1, commandParts.end()};
                m_CommandMap.at(command)(args);
            }
            else
            {
                std::cout << "Unknown command: " << command << std::endl;
                std::cout << "Type \"help\" for available commands" << std::endl;
            }
        }
    }

    void CommandManager::Help()
    {
        std::cout << "Available Commands:" << std::endl;
        std::cout << "* uci" << std::endl;
        std::cout << "\tPrint engine info and options." << std::endl;
        std::cout << "* isready" << std::endl;
        std::cout << "\tCheck if the engine is ready." << std::endl;
        std::cout << "* ucinewgame" << std::endl;
        std::cout << "\tInform the engine that this is a new game." << std::endl;
        std::cout << "* setoption name <name> [value <value>]" << std::endl;
        std::cout << "\tSetup engine parameters using name/value pairs. Some options do not require a value."
                  << std::endl;
        std::cout << "\tExample: setoption name multipv value 3" << std::endl;
        std::cout << "\tSupported options:" << std::endl;
        std::cout << "\t* multipv <count>" << std::endl;
        std::cout
          << "\t\tInstructs the engine to report the top <count> best moves. For best performance set this to 1."
          << std::endl;
        std::cout << "\t* Skill Level <level>" << std::endl;
        std::cout << "\t\tAn integer between 1-20 indicating the skill level the engine should play at. (default 20)"
                  << std::endl;
        std::cout << "\t* Threads" << std::endl;
        std::cout << "\t\tNumber of threads to use during search, for best performance set to the number of CPU cores "
                     "available. (default 1)"
                  << std::endl;
        std::cout << "\t* Hash" << std::endl;
        std::cout << "\t\tSize of the hash table in MB. (default 32)" << std::endl;
        std::cout << "\t* Use NNUE" << std::endl;
        std::cout << "\t\tEnable/Disable the use of NNUE evaluation. (default true)" << std::endl;
        std::cout << "* d" << std::endl;
        std::cout << "\tPrint the current position." << std::endl;
        std::cout << "* position [fen <fenstring> | startpos] [moves <moves>...]" << std::endl;
        std::cout << "\tSet the current position from a FEN string or to the starting position." << std::endl;
        std::cout << "\tAdditionally, apply a list of moves to the given position." << std::endl;
        std::cout << "\tIf fen or startpos is not provided the moves will be applied to the current position."
                  << std::endl;
        std::cout << "* eval" << std::endl;
        std::cout << "\tPrint the static evaluation for the current position." << std::endl;
        std::cout << "* perft <depth>" << std::endl;
        std::cout << "\tPerformance test move generation for a given depth in the current position." << std::endl;
        std::cout << "* go" << std::endl;
        std::cout << "\tMain command to begin searching in the current position." << std::endl;
        std::cout << "\tAccepts a number of different arguments:" << std::endl;
        std::cout << "\t* ponder" << std::endl;
        std::cout << "\t\tStart searching in ponder mode, searching infinitely until a stop command." << std::endl;
        std::cout << "\t* depth <depth>" << std::endl;
        std::cout << "\t\tSearch the current position to a given depth." << std::endl;
        std::cout << "\t* movetime <time_ms>" << std::endl;
        std::cout << "\t\tSearch the current position for a given number of milliseconds." << std::endl;
        std::cout << "\t* wtime <time_ms> btime <time_ms> [winc <time_ms>] [binc <time_ms>] [movestogo <moves_left>]"
                  << std::endl;
        std::cout << "\t\tSearch the current position given a time control. Used in tournament play." << std::endl;
        std::cout << "\t* infinite" << std::endl;
        std::cout << "\t\tSearch the current position until told to stop." << std::endl;
        std::cout << "* moves" << std::endl;
        std::cout << "\tShow information about the legal moves in the current position." << std::endl;
        std::cout << "* undo" << std::endl;
        std::cout << "\tUndo the last played move." << std::endl;
        std::cout << "* nnue" << std::endl;
        std::cout << "\tPrint information about the current network." << std::endl;
        std::cout << "* stop" << std::endl;
        std::cout << "\tStop searching as soon as possible." << std::endl;
        std::cout << "* quit" << std::endl;
        std::cout << "\tQuit the program as soon as possible." << std::endl;
        std::cout << "Examples:" << std::endl;
        std::cout << "\tisready" << std::endl;
        std::cout << "\tposition startpos moves e2e4 e7e5 g1f3" << std::endl;
        std::cout << "\tgo movetime 3000" << std::endl;
    }

    void CommandManager::IsReady()
    {
        std::cout << "readyok" << std::endl;
    }

    void CommandManager::Uci()
    {
        std::cout << "id name Storm" << std::endl;
        std::cout << "id author Jordan Morrison" << std::endl;
        std::cout << "option name MultiPV type spin default 1 min 1 max 100" << std::endl;
        std::cout << "option name Skill Level type spin default 20 min 1 max 20" << std::endl;
        std::cout << "option name Threads type spin default 1 min 1 max 256" << std::endl;
        std::cout << "option name Hash type spin default 32 min 1 max 262144" << std::endl;
        std::cout << "option name Use NNUE type check default true" << std::endl;

        std::cout << "uciok" << std::endl;
    }

    void CommandManager::NewGame()
    {
        if (!m_Searching)
        {
            m_Search.Reset();
            m_CurrentPosition = CreateStartingPosition();
            m_CurrentPosition.SetNetworkEnabled(m_UsingNNUE);
        }
    }

    void CommandManager::PrintBoard()
    {
        std::cout << m_CurrentPosition << std::endl;
        std::cout << "FEN: " << GetFENFromPosition(m_CurrentPosition) << std::endl;
        std::cout << "Hash: " << std::hex << m_CurrentPosition.Hash.Hash << std::dec << std::endl;
        std::cout << "Known Draw: " << (InsufficientMaterial(m_CurrentPosition) ? "true" : "false") << std::endl;
    }

    void CommandManager::SetOption(std::string name, const std::string* value)
    {
        std::transform(name.begin(), name.end(), name.begin(), [](char c) { return std::tolower(c); });
        if (name == "multipv" && value != nullptr)
        {
            m_Settings.MultiPv = std::max(1, std::stoi(*value));
        }
        else if (name == "skill level" && value != nullptr)
        {
            m_Settings.SkillLevel = std::min(std::max(0, std::stoi(*value)), 20);
        }
        else if (name == "threads" && value != nullptr)
        {
            m_Settings.Threads = std::min(std::max(1, std::stoi(*value)), 256);
        }
        else if (name == "hash" && value != nullptr)
        {
            m_Settings.HashBytes = size_t(std::stoi(*value)) * 1024ULL * 1024ULL;
        }
        else if (name == "use nnue" && value != nullptr)
        {
            std::string lowercaseValue = *value;
            std::transform(lowercaseValue.begin(),
              lowercaseValue.end(),
              lowercaseValue.begin(),
              [](char c) { return std::tolower(c); });
            if (lowercaseValue == "true")
            {
                if (IsNNUEAvailable())
                {
                    m_UsingNNUE = true;
                    m_CurrentPosition.SetNetworkEnabled(m_UsingNNUE);
                    m_Search.Reset();
                }
                else
                {
                    std::cout << "Failed to load NNUE, unable to enable NNUE evaluation." << std::endl;
                }
            }
            else if (lowercaseValue == "false")
            {
                m_UsingNNUE = false;
                m_CurrentPosition.SetNetworkEnabled(m_UsingNNUE);
                m_Search.Reset();
            }
            else
            {
                std::cout << "Invalid option value: " << *value << std::endl;
            }
        }
        /*if (name == "book")
		{
			m_OpeningBook.Clear();
			if (value != nullptr && m_OpeningBook.AppendFromFile(*value))
				m_Search.SetOpeningBook(&m_OpeningBook);
			else
			{
				if (value != nullptr)
					std::cout << "Book file not found: " << *value << std::endl;
				std::cout << "Disabling opening book..." << std::endl;
				m_Search.SetOpeningBook(nullptr);
			}
		}*/
        m_Search.SetSettings(m_Settings);
    }

    void CommandManager::SetPositionFen(const std::string& fen)
    {
        if (!m_Searching)
        {
            // m_Search.Reset();
            m_CurrentPosition = CreatePositionFromFEN(fen);
            m_CurrentPosition.SetNetworkEnabled(m_UsingNNUE);
        }
    }

    void CommandManager::ApplyMoves(const std::vector<std::string>& moves)
    {
        Move moveBuffer[MAX_MOVES];
        if (!m_Searching)
        {
            for (const std::string& moveString : moves)
            {
                Move* it = moveBuffer;
                Move* end;
                if (m_CurrentPosition.ColorToMove == COLOR_WHITE)
                    end = GenerateAll<COLOR_WHITE, ALL>(m_CurrentPosition, moveBuffer);
                else
                    end = GenerateAll<COLOR_BLACK, ALL>(m_CurrentPosition, moveBuffer);

                Move move = UCI::CreateMoveFromString(m_CurrentPosition, moveString);

                bool legal = false;
                while (it != end)
                {
                    if (move == *it)
                    {
                        if (m_CurrentPosition.IsLegal(move))
                        {
                            StateInfo st;
                            m_CurrentPosition.ApplyMove(move, st, &m_Undo);
                            m_UndoMove = move;
                            m_Search.PushPosition(m_CurrentPosition.Hash);
                            legal = true;
                        }
                        break;
                    }
                    it++;
                }
                if (!legal)
                {
                    std::cout << "Move " << moveString << " is not valid." << std::endl;
                    break;
                }
            }
        }
    }

    void CommandManager::Eval()
    {
        StateInfo st;
        m_CurrentPosition.Reset(&st);

        if (m_UsingNNUE)
        {
            std::cout << FormatNNUEEvaluation(m_CurrentPosition) << std::endl;
            if (m_CurrentPosition.IsNetworkEnabled())
                std::cout << "NNUE evaluation: "
                          << NNUE::EvaluateNNUE(m_CurrentPosition, true) *
                               (m_CurrentPosition.ColorToMove == COLOR_WHITE ? 1 : -1)
                          << " (white side)" << std::endl;
        }
        else
        {
            EvaluationResult evaluation = EvaluateDetailed(m_CurrentPosition);
            std::cout << FormatEvaluation(evaluation) << std::endl;
        }
    }

    void CommandManager::Perft(int depth)
    {
        if (!m_Searching)
        {
            StateInfo st;
            m_CurrentPosition.Reset(&st);
            m_Search.Perft(m_CurrentPosition, depth);
        }
    }

    void CommandManager::Go(const std::vector<std::string>& args)
    {
        if (args.size() > 0)
        {
            SearchLimits limits;
            m_CurrentPosition.Reset(&m_StateInfo);

            size_t moveStartIndex = 0;
            for (size_t i = 0; i < args.size(); i++)
            {
                const std::string& token = args[i];
                if (token == "wtime" && i + 1 < args.size())
                    limits.WhiteTime = std::stoi(args[++i]);
                else if (token == "btime" && i + 1 < args.size())
                    limits.BlackTime = std::stoi(args[++i]);
                else if (token == "winc" && i + 1 < args.size())
                    limits.WhiteIncrement = std::stoi(args[++i]);
                else if (token == "binc" && i + 1 < args.size())
                    limits.BlackIncrement = std::stoi(args[++i]);
                else if (token == "movestogo" && i + 1 < args.size())
                    limits.MovesToGo = std::stoi(args[++i]);

                else if (token == "movetime" && i + 1 < args.size())
                    limits.Milliseconds = std::stoi(args[++i]);
                else if (token == "depth" && i + 1 < args.size())
                    limits.Depth = std::stoi(args[++i]);
                else if (token == "nodes" && i + 1 < args.size())
                    limits.Nodes = std::stoi(args[++i]);

                else if (token == "infinite" || token == "ponder")
                    limits.Infinite = true;
                else
                {
                    moveStartIndex = i;
                    break;
                }
            }

            if (!m_Searching)
            {
                std::unordered_set<Move> includedMoves =
                  moveStartIndex > 0 ? GetMoveList(args, moveStartIndex) : std::unordered_set<Move> {};
                limits.Only = includedMoves;
                m_Searching = true;
                if (m_SearchThread.joinable())
                    m_SearchThread.join();
                m_SearchThread = std::thread(
                  [this, limits]()
                  {
                      BestMove bestMove = m_Search.SearchBestMove(m_CurrentPosition, limits);
                      std::cout << "bestmove " << UCI::FormatMove(bestMove.Move);
                      if (bestMove.PonderMove != MOVE_NONE)
                          std::cout << " ponder " << UCI::FormatMove(bestMove.PonderMove);
                      std::cout << std::endl;
                      m_Searching = false;
                  });
            }
        }
    }

    void CommandManager::Moves()
    {
        for (Move mv : GetLegalMoves(m_CurrentPosition))
        {
            std::cout << UCI::FormatMove(mv) << std::endl;
        }
        return;

        Move moveBuffer[MAX_MOVES];
        Move* it = moveBuffer;
        Move* end;
        if (m_CurrentPosition.ColorToMove == COLOR_WHITE)
            end = GenerateAll<COLOR_WHITE, ALL>(m_CurrentPosition, moveBuffer);
        else
            end = GenerateAll<COLOR_BLACK, ALL>(m_CurrentPosition, moveBuffer);
        while (it != end)
        {
            if (m_CurrentPosition.IsLegal(*it))
                std::cout << std::boolalpha << UCI::FormatMove(*it) << " Check: " << m_CurrentPosition.GivesCheck(*it)
                          << ", SEE (>= 0): " << m_CurrentPosition.SeeGE(*it) << std::endl;
            it++;
        }
    }

    void CommandManager::Probe()
    {
        auto collection = m_OpeningBook.Probe(m_CurrentPosition.Hash);
        if (collection)
        {
            auto sortedEntries = collection->Entries;
            std::sort(sortedEntries.begin(),
              sortedEntries.end(),
              [](const BookEntry& a, const BookEntry& b) { return a.Count > b.Count; });
            std::cout << "Book Moves:" << std::endl;
            for (const BookEntry& entry : sortedEntries)
            {
                Move move = CreateMoveFromEntry(entry, m_CurrentPosition);
                std::cout << UCI::FormatMove(move) << " " << PGN::FormatMove(move, m_CurrentPosition)
                          << " Count: " << entry.Count << std::endl;
            }
            std::cout << "Total Count: " << collection->TotalCount << std::endl;
        }
        else
        {
            std::cout << "No book entry found" << std::endl;
        }

        /*const TranspositionTable& tt = m_Search.GetTranspositionTable();
		bool ttHit;
		TranspositionTableEntry* entry = tt.GetEntry(m_CurrentPosition.Hash, ttHit);
		if (ttHit)
		{
			std::cout << "Transposition Table Entry:" << std::endl;
			std::cout << "Depth: " << entry->GetDepth() << std::endl;
			std::cout << "Move: " << UCI::FormatMove(entry->GetMove()) << std::endl;
			std::cout << "Score: " << entry->GetValue() << std::endl;
			std::cout << "Bound: " << (entry->GetBound() == BOUND_EXACT ? "EXACT" : entry->GetBound() == BOUND_UPPER ? "UPPER" : "LOWER") << std::endl;
		}
		else
		{
			std::cout << "No transposition table entry found. Entry hash: " << std::hex << entry->GetHash().Hash << std::dec << std::endl;
		}*/
    }

    void CommandManager::ProbeTT()
    {
        const TranspositionTable& tt = m_Search.GetTranspositionTable();
        bool ttHit;
        TranspositionTableEntry* entry = tt.GetEntry(m_CurrentPosition.Hash, ttHit);
        if (ttHit)
        {
            std::cout << "Transposition Table Entry:" << std::endl;
            std::cout << "Depth: " << entry->GetDepth() << std::endl;
            std::cout << "Move: " << UCI::FormatMove(entry->GetMove()) << std::endl;
            std::cout << "Score: " << entry->GetValue() << std::endl;
            std::cout << "Bound: "
                      << (entry->GetBound() == BOUND_EXACT  ? "EXACT" :
                           entry->GetBound() == BOUND_UPPER ? "UPPER" :
                                                              "LOWER")
                      << std::endl;
        }
        else
        {
            std::cout << "No transposition table entry found. Entry hash: " << std::hex << entry->GetHash().Hash
                      << std::dec << std::endl;
        }
    }

    void CommandManager::Stop()
    {
        if (m_Searching)
        {
            m_Search.Stop();
            m_SearchThread.join();
        }
    }

    void CommandManager::Quit()
    {
        Stop();
        exit(0);
    }

    std::unordered_set<Move> CommandManager::GetMoveList(const std::vector<std::string>& args, int offset) const
    {
        std::unordered_set<Move> includedMoves;
        for (int i = offset; i < args.size(); i++)
        {
            Move mv = UCI::CreateMoveFromString(m_CurrentPosition, args[i]);
            if (mv != MOVE_NONE)
                includedMoves.insert(mv);
        }
        return includedMoves;
    }

}
