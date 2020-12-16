#include "Search.h"
#include <chrono>

namespace Storm
{

    void UpdatePV(Move* pv, Move move, Move* childPv)
    {
        for (*pv++ = move; childPv && *childPv != MOVE_NONE; )
            *pv++ = *childPv++;
        *pv = MOVE_NONE;
    }

    Search::Search(size_t ttSize, bool log)
        : m_TranspositionTable(ttSize), m_Log(log), m_Limits(), m_RootMoves(), m_Nodes(0), m_StartRootTime(), m_StartSearchTime(), m_Stopped(false), m_ShouldStop(false)
    {
    }

    size_t Search::Perft(const Position& position, int depth)
    {
        UndoInfo undo;
        size_t total = 0;
        Move moves[MAX_MOVES];
        MoveList moveList(moves);
        moveList.FillLegal<ALL>(position);

        auto startTime = std::chrono::high_resolution_clock::now();

        for (Move move : moveList)
        {
            Position movedPosition = position;
            movedPosition.ApplyMove(move, &undo);
            size_t perft = PerftPosition(movedPosition, depth - 1);
            total += perft;

            if (m_Log)
                std::cout << UCI::FormatMove(move) << ": " << perft << std::endl;
        }
        if (m_Log)
        {
            auto endTime = std::chrono::high_resolution_clock::now();
            auto elapsed = endTime - startTime;
            std::cout << "====================================" << std::endl;
            std::cout << "Total Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() << "ms" << std::endl;
            std::cout << "Total Nodes: " << total << std::endl;
            std::cout << "Nodes per Second: " << (size_t)(total / (double)std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count() * 1e9) << std::endl;
        }
        return total;
    }

    void Search::Ponder(const Position& position, SearchLimits limits)
    {
        limits.Infinite = true;
        SearchBestMove(position, limits);
    }

    Move Search::SearchBestMove(const Position& position, SearchLimits limits)
    {
        Position pos = position;
        m_Limits = limits;
        int depth = limits.Depth < 0 ? MAX_PLY : limits.Depth;
        RootMove move = SearchRoot(pos, depth);
        if (move.Pv.size() > 0)
            return move.Pv[0];
        return MOVE_NONE;
    }

    void Search::Stop()
    {
        m_ShouldStop = true;
    }

    size_t Search::PerftPosition(Position& position, int depth)
    {
        if (depth <= 0)
            return 1;
        Move moves[MAX_MOVES];
        Move* it = moves;
        Move* end;
        if (position.ColorToMove == COLOR_WHITE)
            end = GenerateAllLegal<COLOR_WHITE, ALL>(position, moves);
        else
            end = GenerateAllLegal<COLOR_BLACK, ALL>(position, moves);
        UndoInfo undo;
        if (depth == 1)
        {
            return end - it;
        }
        size_t nodes = 0;
        while (it != end)
        {
            Position movedPosition = position;
            movedPosition.ApplyMove(*it++, &undo);
            nodes += PerftPosition(movedPosition, depth - 1);
        }
        return nodes;
    }

    RootMove Search::SearchRoot(Position& position, int depth)
    {
        m_RootMoves = GenerateRootMoves(position);
        if (m_RootMoves.empty())
            return {};

        m_Stopped = false;
        m_ShouldStop = false;
        m_StartRootTime = std::chrono::high_resolution_clock::now();
        int rootDepth = 1;

        Move pv[MAX_PLY];
        pv[0] = MOVE_NONE;
        SearchStack stack[MAX_PLY + 10];
        SearchStack* stackPtr = stack;

        stackPtr->Ply = 0;
        stackPtr->PV = pv;

        int selDepth = 0;
        ValueType alpha = -VALUE_MATE;
        ValueType beta = VALUE_MATE;
        ValueType bestValue = -VALUE_MATE;
        
        while (rootDepth <= depth)
        {
            m_Nodes = 0;
            m_StartSearchTime = std::chrono::high_resolution_clock::now();

            ValueType value = SearchPosition<PV>(position, stackPtr, rootDepth, alpha, beta, selDepth, false);
            bestValue = value;

            std::stable_sort(m_RootMoves.begin(), m_RootMoves.end());

            if (CheckLimits())
            {
                m_Stopped = true;
                break;
            }

            RootMove& rootMove = m_RootMoves[0];

            if (m_Log)
            {
                auto elapsed = std::chrono::high_resolution_clock::now() - m_StartSearchTime;
                std::cout << "info depth " << rootDepth << " seldepth " << rootMove.SelDepth << " score ";
                if (!IsMateScore(rootMove.Score))
                {
                    std::cout << "cp " << rootMove.Score;
                }
                else
                {
                    if (rootMove.Score > 0)
                        std::cout << "mate " << (GetPliesFromMateScore(rootMove.Score) / 2 + 1);
                    else
                        std::cout << "mate " << -(GetPliesFromMateScore(rootMove.Score) / 2);
                }
                std::cout << " nodes " << m_Nodes;
                std::cout << " nps " << (size_t)(m_Nodes / (elapsed.count() / 1e9f));
                std::cout << " time " << (size_t)(elapsed.count() / 1e6f);
                std::cout << " multipv " << 1;
                //if (hashFull >= 500)
                //    std::cout << " hashfull " << hashFull;
                std::cout << " pv";
                for (Move move : rootMove.Pv)
                {
                    std::cout << " " << UCI::FormatMove(move);
                }
                std::cout << std::endl;
            }

            rootDepth++;
        }

        int selectedIndex = 0;
        return m_RootMoves[selectedIndex];
    }

    template<Search::NodeType NT>
    inline ValueType Search::SearchPosition(Position& position, SearchStack* stack, int depth, ValueType alpha, ValueType beta, int& selDepth, bool cutNode)
    {
        constexpr int FirstMoveIndex = 1;
        constexpr bool IsPvNode = NT == PV;
        const bool IsRoot = IsPvNode && stack->Ply == 0;
        const bool inCheck = position.InCheck();

        Move pv[MAX_PLY];
        UndoInfo undo;

        (stack + 1)->Ply = stack->Ply + 1;
        pv[0] = MOVE_NONE;
        (stack + 1)->PV = pv;

        ValueType originalAlpha = alpha;

        if (stack->Ply > selDepth)
            selDepth = stack->Ply;

        if (stack->Ply >= MAX_PLY)
            return VALUE_DRAW;

        if (depth <= 0)
            return QuiescenceSearch<NT>(position, stack, depth, alpha, beta, cutNode);

        bool ttHit;
        ZobristHash ttHash = position.Hash;
        TranspositionTableEntry* ttEntry = m_TranspositionTable.GetEntry(ttHash, ttHit);

        ValueType ttValue = ttHit ? ttEntry->GetValue() : VALUE_NONE;
        Move ttMove = ttHit ? ttEntry->GetMove() : MOVE_NONE;

        if (!IsRoot && ttHit && ttEntry->GetDepth() >= depth)
        {
            EntryBound bound = ttEntry->GetBound();
            if (bound == BOUND_EXACT)
                return ttValue;
            if (bound & BOUND_LOWER)
                alpha = std::max(ttValue, alpha);
            else if (bound & BOUND_UPPER)
                beta = std::min(beta, ttValue);
            if (alpha >= beta)
                return alpha;
        }

        Move moves[MAX_MOVES];
        Move* it = moves;
        Move* end;

        if (position.ColorToMove == COLOR_WHITE)
            end = GenerateAll<COLOR_WHITE, ALL>(position, moves);
        else
            end = GenerateAll<COLOR_BLACK, ALL>(position, moves);

        ValueType bestValue = -VALUE_MATE;
        Move bestMove = MOVE_NONE;
        int moveIndex = 0;

        while (it != end)
        {
            if (position.IsLegal(*it))
            {
                moveIndex++;
                bool givesCheck = position.GivesCheck(*it);
                Position movedPosition = position;
                movedPosition.ApplyMove(*it, &undo);
                m_Nodes++;

                pv[0] = MOVE_NONE;
                ValueType value = -SearchPosition<NT>(movedPosition, stack + 1, depth - 1, -beta, -alpha, selDepth, cutNode);

                if (CheckLimits())
                {
                    m_Stopped = true;
                    return VALUE_NONE;
                }

                if (IsRoot)
                {
                    RootMove& rootMove = *std::find(m_RootMoves.begin(), m_RootMoves.end(), *it);
                    if (moveIndex == FirstMoveIndex || (value > alpha))
                    {
                        rootMove.Score = value;
                        rootMove.SelDepth = selDepth;
                        rootMove.Pv.resize(1);
                        for (Move* m = (stack + 1)->PV; *m != MOVE_NONE; ++m)
                            rootMove.Pv.push_back(*m);
                    }
                    else
                        rootMove.Score = -VALUE_MATE;
                }

                if (value > bestValue)
                {
                    bestValue = value;
                    bestMove = *it;
                    if (value > alpha)
                    {
                        alpha = value;
                        if (IsPvNode)
                            UpdatePV(stack->PV, *it, (stack + 1)->PV);
                    }
                }
                if (value >= beta)
                {
                    ttEntry->Update(ttHash, *it, depth, BOUND_LOWER, value);
                    return value;
                }
            }
            ++it;
        }

        if (bestValue == -VALUE_MATE)
        {
            if (inCheck)
                return MatedIn(stack->Ply);
            return VALUE_DRAW;
        }

        ttEntry->Update(ttHash, bestMove, depth, alpha > originalAlpha ? BOUND_EXACT : BOUND_UPPER, bestValue);

        return bestValue;
    }

    template<Search::NodeType NT>
    inline ValueType Search::QuiescenceSearch(Position& position, SearchStack* stack, int depth, ValueType alpha, ValueType beta, bool cutNode)
    {
        constexpr bool IsPvNode = NT == PV;
        const bool inCheck = position.InCheck();
        Move pv[MAX_PLY];
        UndoInfo undo;

        if (IsDraw(position, stack))
            return VALUE_DRAW;

        ValueType evaluation = Evaluate(position);

        if (stack->Ply >= MAX_PLY)
            return evaluation;

        if (!inCheck)
        {
            if (evaluation >= beta)
                return evaluation;
            if (alpha < evaluation)
                alpha = evaluation;
        }

        (stack + 1)->Ply = stack->Ply + 1;
        (stack + 1)->PV = pv;

        Move moves[MAX_MOVES];
        Move* it = moves;
        Move* end;

        if (inCheck)
        {
            if (position.ColorToMove == COLOR_WHITE)
                end = GenerateAll<COLOR_WHITE, CAPTURES | EVASIONS>(position, moves);
            else
                end = GenerateAll<COLOR_BLACK, CAPTURES | EVASIONS>(position, moves);
        }
        else
        {
            if (position.ColorToMove == COLOR_WHITE)
                end = GenerateAll<COLOR_WHITE, CAPTURES>(position, moves);
            else
                end = GenerateAll<COLOR_BLACK, CAPTURES>(position, moves);
        }

        ValueType bestValue = -VALUE_MATE;
        int moveIndex = 0;

        while (it != end)
        {
            if (position.IsLegal(*it))
            {
                moveIndex++;
                bool givesCheck = position.GivesCheck(*it);
                Position movedPosition = position;
                movedPosition.ApplyMove(*it, &undo);
                m_Nodes++;

                pv[0] = MOVE_NONE;
                ValueType value = -QuiescenceSearch<NT>(movedPosition, stack + 1, depth - 1, -beta, -alpha, cutNode);

                if (CheckLimits())
                {
                    m_Stopped = true;
                    return VALUE_NONE;
                }

                if (value > bestValue)
                {
                    bestValue = value;
                    if (value > alpha)
                    {
                        alpha = value;
                        if (IsPvNode)
                            UpdatePV(stack->PV, *it, (stack + 1)->PV);
                    }
                }
                if (value >= beta)
                    return value;
            }
            ++it;
        }

        if (inCheck && bestValue == -VALUE_MATE)
            return MatedIn(stack->Ply);

        return alpha;
    }

    bool Search::IsDraw(const Position& position, SearchStack* stack) const
    {
        if (position.HalfTurnsSinceCaptureOrPush >= 100)
            return true;
        return false;
    }

    bool Search::CheckLimits() const
    {
        if (m_Limits.Infinite)
            return m_ShouldStop || m_Stopped;
        if (m_Nodes & 1023)
        {
            if (m_Limits.Milliseconds > 0)
            {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - m_StartRootTime);
                if (elapsed.count() >= m_Limits.Milliseconds)
                    return true;
            }
            if (m_Limits.Nodes > 0)
            {
                return m_Nodes >= m_Limits.Nodes;
            }
        }
        return m_ShouldStop || m_Stopped;
    }

    std::vector<RootMove> Search::GenerateRootMoves(const Position& position) const
    {
        Move buffer[MAX_MOVES];
        MoveList list(buffer);
        list.FillLegal<ALL>(position);
        std::vector<RootMove> result;
        for (Move move : list)
            result.push_back({ std::vector<Move>{ move }, VALUE_DRAW, 0 });
        return result;
    }

}
