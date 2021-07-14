#include "Searcher.h"

namespace Storm
{

    Searcher::Searcher(const std::string& engineCommand, ThreadSafeFileWriter& writer, SharedData& sharedData, int64_t iterations, int depth, size_t seed)
        : m_Engine(engineCommand), m_Writer(writer), m_SharedData(sharedData), m_Iterations(iterations), m_CompletedIterations(0), m_Depth(depth), m_BestMoveChance(0.5f), m_WrittenFens(), m_StringCache(), m_Random(), m_EvalLimit(3000)
    {
        m_Random.seed(seed);
    }

    void Searcher::Start(const Position& initialPosition, bool onlyQuiet)
    {
        Position position = initialPosition;
        while (m_Iterations > 0)
        {
            SearchPosition(position, 0, onlyQuiet);
        }
        FlushData();
    }

    Searcher::SearchResult Searcher::SearchPosition(Position& position, int ply, bool onlyQuiet)
    {
        const bool IsRoot = ply == 0;
        SearchResult result;
        result.Continue = true;

        // Always a chance to skip this position
        if (m_Iterations > 0 && (IsRoot || GetRandom() > 0.05f))
        {
            // If we are past the 50 move rule have some probability to restart
            if (position.HalfTurnsSinceCaptureOrPush >= 100)
            {
                result.Continue = GetRandom() < 0.2f;
            }
            else
            {
                Move moveBuffer[MAX_MOVES];
                MoveList moveList(moveBuffer);
                moveList.FillLegal<ALL>(position);
                std::vector<Move> moves = { moveList.begin(), moveList.end() };

                if (moves.size() > 0)
                {
                    std::string fen = GetFENFromPosition(position);
                    PositionResult data = m_Engine.AnalyzePosition(fen, m_Depth, false);
                    if (data.Mate != 0)
                    {
                        result.Continue = GetRandom() < 0.5f;
                    }
                    else if (std::abs(data.Score) <= m_EvalLimit)
                    {
                        // Ensure that the position is quiet (not in check and no captures)
                        if (!onlyQuiet || (!position.InCheck() && std::find_if(moves.begin(), moves.end(), [&position](const Move& move) { return position.IsCapture(move) && position.SeeGE(move); }) == moves.end()))
                            WriteData(fen, data);
                        Move bestMove = UCI::CreateMoveFromString(position, data.BestMove);
                        Move move;
                        bool doneBestMove = false;
                        while (moves.size() > 0)
                        {
                            if (GetRandom() < m_BestMoveChance && !doneBestMove)
                            {
                                // Use the best move
                                move = bestMove;
                                doneBestMove = true;
                                moves.erase(std::find(moves.begin(), moves.end(), move));
                            }
                            else
                            {
                                move = PickRandom(moves);
                                if (move == bestMove)
                                    doneBestMove = true;
                            }
                            UndoInfo undo;
                            position.ApplyMove(move, &undo);
                            SearchResult searchResult = SearchPosition(position, ply + 1, onlyQuiet);
                            position.UndoMove(move, undo);

                            if (!searchResult.Continue || m_Iterations <= 0)
                            {
                                result.Continue = false;
                                break;
                            }
                        }
                    }
                    else
                    {
                        result.Continue = GetRandom() < 0.6f;
                    }
                }
            }
        }

        return result;
    }

    float Searcher::GetRandom() const
    {
        std::uniform_real_distribution dist(0.0f, 1.0f);
        return dist(m_Random);
    }

    Move Searcher::PickRandom(std::vector<Move>& moves) const
    {
        if (moves.size() > 0)
        {
            std::uniform_int_distribution dist(0, int(moves.size() - 1));
            auto iterator = moves.begin() + dist(m_Random);
            Move move = *iterator;
            moves.erase(iterator);
            return move;
        }
        return MOVE_NONE;
    }

    std::string Searcher::GetComparableFen(const std::string& fen) const
    {
        size_t lastSpace = fen.find_last_of(' ');
        size_t secondLastSpace = fen.find_last_of(' ', lastSpace - 1);
        return fen.substr(0, secondLastSpace);
    }

    void Searcher::WriteData(const std::string& fen, const PositionResult& result)
    {
        std::string comparableFen = GetComparableFen(fen);
        if (m_WrittenFens.find(comparableFen) == m_WrittenFens.end())
        {
            m_WrittenFens.insert(comparableFen);
            m_StringCache.push_back(std::to_string(result.Score) + " " + comparableFen + "\n");
            m_Iterations--;
            m_CompletedIterations++;
            if (m_StringCache.size() >= 1000)
            {
                FlushData();
            }
        }
    }

    void Searcher::FlushData()
    {
        for (const auto& string : m_StringCache)
            m_Writer.WriteString(string);
        m_StringCache.clear();

        std::scoped_lock<std::mutex> lock(m_SharedData.Mutex);
        m_SharedData.CompletedIterations += m_CompletedIterations;
        std::cout << m_SharedData.CompletedIterations << " / " << m_SharedData.TotalIterations << std::endl;
        m_CompletedIterations = 0;

        if (m_WrittenFens.size() > 100000)
            m_WrittenFens.clear();
    }

}
