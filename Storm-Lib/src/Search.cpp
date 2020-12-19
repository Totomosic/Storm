#include "Search.h"
#include <chrono>

namespace Storm
{

    int LmrReductions[MAX_PLY][MAX_MOVES];

    void InitSearch()
    {
        for (int depth = 0; depth < MAX_PLY; depth++)
        {
            for (int index = 0; index < MAX_MOVES; index++)
            {
                LmrReductions[depth][index] = int(std::round(1.0 + std::log(depth) + std::log(index) * 0.5));
            }
        }
    }

    void UpdatePV(Move* pv, Move move, Move* childPv)
    {
        for (*pv++ = move; childPv && *childPv != MOVE_NONE; )
            *pv++ = *childPv++;
        *pv = MOVE_NONE;
    }

    Search::Search(size_t ttSize, bool log)
        : m_TranspositionTable(ttSize), m_PositionHistory(), m_Log(log), m_Limits(), m_RootMoves(), m_Nodes(0), m_StartRootTime(), m_StartSearchTime(), m_Stopped(false), m_ShouldStop(false), m_CounterMoves()
    {
        ClearTable<Move, SQUARE_MAX, SQUARE_MAX>(m_CounterMoves, MOVE_NONE);
    }

    void Search::PushPosition(const ZobristHash& hash)
    {
        m_PositionHistory.push_back(hash);
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
        ZobristHash* positionHistory = new ZobristHash[m_PositionHistory.size() + MAX_PLY];
        pv[0] = MOVE_NONE;
        SearchStack stack[MAX_PLY + 10];
        SearchStack* stackPtr = stack;
        ZobristHash* historyPtr = positionHistory;

        for (int i = 0; i < m_PositionHistory.size(); i++)
        {
            *historyPtr++ = m_PositionHistory[i];
        }

        stackPtr->Ply = -2;
        stackPtr->PV = pv;
        stackPtr->Killers[0] = stackPtr->Killers[1] = MOVE_NONE;
        stackPtr->CurrentMove = MOVE_NONE;
        stackPtr->StaticEvaluation = VALUE_NONE;
        stackPtr->PositionHistory = historyPtr;

        stackPtr++;

        stackPtr->Ply = -1;
        stackPtr->PV = pv;
        stackPtr->Killers[0] = stackPtr->Killers[1] = MOVE_NONE;
        stackPtr->CurrentMove = MOVE_NONE;
        stackPtr->StaticEvaluation = VALUE_NONE;
        stackPtr->PositionHistory = historyPtr;

        stackPtr++;

        stackPtr->Ply = 0;
        stackPtr->PV = pv;
        stackPtr->Killers[0] = stackPtr->Killers[1] = MOVE_NONE;
        stackPtr->CurrentMove = MOVE_NONE;
        stackPtr->StaticEvaluation = VALUE_NONE;
        stackPtr->PositionHistory = historyPtr;

        (stackPtr + 1)->Killers[0] = (stackPtr + 1)->Killers[1] = MOVE_NONE;

        int selDepth = 0;
        ValueType alpha = -VALUE_MATE;
        ValueType beta = VALUE_MATE;
        ValueType bestValue = -VALUE_MATE;

        RootMove bestMove;
        
        while (rootDepth <= depth)
        {
            m_Nodes = 0;
            m_StartSearchTime = std::chrono::high_resolution_clock::now();

            ValueType delta = 0;
            if (depth >= AspirationWindowDepth)
            {
                delta = 20;
                alpha = std::max(bestValue - delta, -VALUE_MATE);
                beta = std::min(bestValue + delta, VALUE_MATE);
            }

            while (true)
            {
                ValueType value = SearchPosition<PV>(position, stackPtr, rootDepth, alpha, beta, selDepth, false);
                bestValue = value;

                std::stable_sort(m_RootMoves.begin(), m_RootMoves.end());

                if (value <= alpha && value != -VALUE_MATE)
                {
                    beta = (alpha + beta) / 2;
                    alpha = std::max(value - delta, -VALUE_MATE);
                }
                else if (value >= beta && value != VALUE_MATE)
                {
                    beta = std::min(value + delta, VALUE_MATE);
                }
                else
                    break;

                if (CheckLimits())
                {
                    m_Stopped = true;
                    break;
                }

                delta += delta / 4 + 5;
            }

            if (CheckLimits())
            {
                m_Stopped = true;
                break;
            }

            RootMove& rootMove = m_RootMoves[0];
            bestMove = rootMove;

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

        delete[] positionHistory;
        return bestMove;
    }

    template<Search::NodeType NT>
    inline ValueType Search::SearchPosition(Position& position, SearchStack* stack, int depth, ValueType alpha, ValueType beta, int& selDepth, bool cutNode)
    {
        constexpr int FirstMoveIndex = 1;
        constexpr bool IsPvNode = NT == PV;
        const bool IsRoot = IsPvNode && stack->Ply == 0;
        const bool inCheck = position.InCheck();

        STORM_ASSERT(IsPvNode || alpha == beta - 1, "Invalid alpha beta bounds");
        STORM_ASSERT(alpha < beta, "Invalid alpha beta bounds");
        STORM_ASSERT(!(IsPvNode && cutNode), "Invalid configuration");

        Move pv[MAX_PLY];
        UndoInfo undo;

        (stack + 1)->Ply = stack->Ply + 1;
        pv[0] = MOVE_NONE;
        (stack + 1)->PV = pv;
        (stack + 1)->StaticEvaluation = VALUE_NONE;
        (stack + 1)->PositionHistory = stack->PositionHistory + 1;
        (stack + 2)->Killers[0] = (stack + 2)->Killers[1] = MOVE_NONE;

        stack->PositionHistory[0] = position.Hash;

        ValueType originalAlpha = alpha;

        if (stack->Ply >= MAX_PLY)
            return VALUE_DRAW;

        if (depth <= 0)
            return QuiescenceSearch<NT>(position, stack, depth, alpha, beta, cutNode);

        if (IsDraw(position, stack))
            return VALUE_DRAW;

        if (stack->Ply > selDepth)
            selDepth = stack->Ply;

        // Mate distance pruning
        if (!IsRoot)
        {
            alpha = std::max(MatedIn(stack->Ply), alpha);
            beta = std::min(MateIn(stack->Ply), beta);
            if (alpha >= beta)
                return alpha;
        }

        Move previousMove = (stack - 1)->CurrentMove;

        bool ttHit;
        ZobristHash ttHash = position.Hash;
        TranspositionTableEntry* ttEntry = m_TranspositionTable.GetEntry(ttHash, ttHit);

        ValueType ttValue = IsRoot ? m_RootMoves[0].Score : ttHit ? GetValueFromTT(ttEntry->GetValue(), stack->Ply) : VALUE_NONE;
        Move ttMove = IsRoot ? m_RootMoves[0].Pv[0] : ttHit ? ttEntry->GetMove() : MOVE_NONE;

        if (!IsPvNode && ttHit && ttEntry->GetDepth() >= depth)
        {
            EntryBound bound = ttEntry->GetBound();
            if (bound == BOUND_EXACT)
                return ttValue;
            if (bound & BOUND_LOWER)
            {
                if (!position.IsCapture(ttMove))
                {
                    if (ttMove != stack->Killers[0] && ttMove != stack->Killers[1])
                    {
                        stack->Killers[0] = stack->Killers[1];
                        stack->Killers[1] = ttMove;
                    }
                    m_CounterMoves[GetFromSquare(previousMove)][GetToSquare(previousMove)] = ttMove;
                }
                alpha = std::max(ttValue, alpha);
            }
            else if (bound & BOUND_UPPER)
            {
                beta = std::min(beta, ttValue);
            }
            if (alpha >= beta)
                return alpha;
        }

        if (!inCheck)
        {
            if (ttHit)
            {
                stack->StaticEvaluation = ttValue;
            }
            else
            {
                stack->StaticEvaluation = Evaluate(position);
            }
        }

        bool improving = !inCheck ? stack->StaticEvaluation >= (stack - 2)->StaticEvaluation : false;

        if (!IsPvNode && !inCheck && (stack - 1)->CurrentMove != MOVE_NONE)
        {
            // Razoring
            if (depth <= RazorDepth && stack->StaticEvaluation + RazorMargin < beta)
            {
                ValueType value = QuiescenceSearch<NonPV>(position, stack, 0, alpha, beta, cutNode);
                if (value < beta)
                    return value;
            }

            // Futility Pruning
            if (position.GetNonPawnMaterial() > 0 && depth <= FutilityDepth && stack->StaticEvaluation - GetFutilityMargin(depth) >= beta && !IsMateScore(stack->StaticEvaluation))
            {
                return stack->StaticEvaluation;
            }

            // Null move pruning
            if (depth >= NullMoveDepth && stack->StaticEvaluation >= beta && position.GetNonPawnMaterial() >= 2 * RookValueEg && !IsMateScore(stack->StaticEvaluation))
            {
                Position movedPosition = position;
                movedPosition.ApplyNullMove();

                stack->CurrentMove = MOVE_NONE;

                ValueType value = -SearchPosition<NonPV>(movedPosition, stack + 1, depth - GetNullMoveDepthReduction(depth, stack->StaticEvaluation, beta), -beta, -beta + 1, selDepth, true);
                if (value >= beta)
                    return IsMateScore(value) ? beta : value;
            }
        }

        Move counterMove = m_CounterMoves[GetFromSquare(previousMove)][GetToSquare(previousMove)];
        MoveSelector<ALL_MOVES> selector(position, ttMove, counterMove, stack->Killers);

        ValueType bestValue = -VALUE_MATE;
        Move bestMove = MOVE_NONE;
        int moveIndex = 0;

        Move move;

        while ((move = selector.GetNextMove()) != MOVE_NONE)
        {
            if (IsRoot && std::count(m_RootMoves.begin() + 0, m_RootMoves.end(), move) == 0)
                continue;

            moveIndex++;
            bool givesCheck = position.GivesCheck(move);

            STORM_ASSERT(moveIndex != FirstMoveIndex || ttMove == MOVE_NONE || move == ttMove, "Invalid move ordering");

            if (!position.IsLegal(move))
            {
                moveIndex--;
                continue;
            }

            const bool isCapture = position.IsCapture(move);
            const bool isPromotion = GetMoveType(move) == PROMOTION;
            const bool goodCheck = givesCheck && position.SeeGE(move);

            int depthExtension = 0;

            if (GetMoveType(move) == CASTLE)
                depthExtension = 1;
            else if (goodCheck)
                depthExtension = 1;

            Position movedPosition = position;
            movedPosition.ApplyMove(move, &undo, givesCheck);
            m_Nodes++;

            stack->CurrentMove = move;

            ValueType value;
            int newDepth = depth - 1 + depthExtension;
            bool fullDepthSearch = false;

            if (depth >= LmrDepth && moveIndex > LmrMoveIndex + 2 * IsRoot && !depthExtension)
            {
                int reduction = GetLmrReduction<IsPvNode>(improving, depth, moveIndex);

                if (isCapture || isPromotion)
                    reduction--;

                int lmrDepth = std::clamp(newDepth - reduction, 1, newDepth);
                value = -SearchPosition<NonPV>(movedPosition, stack + 1, lmrDepth, -(alpha + 1), -alpha, selDepth, true);
                fullDepthSearch = value > alpha && lmrDepth != newDepth;
            }
            else
            {
                fullDepthSearch = !IsPvNode || moveIndex > FirstMoveIndex;
            }

            if (fullDepthSearch)
            {
                value = -SearchPosition<NonPV>(movedPosition, stack + 1, newDepth, -(alpha + 1), -alpha, selDepth, !cutNode);
            }
            if (IsPvNode && (moveIndex == FirstMoveIndex || (value > alpha && (IsRoot || value < beta))))
            {
                pv[0] = MOVE_NONE;
                value = -SearchPosition<PV>(movedPosition, stack + 1, newDepth, -beta, -alpha, selDepth, false);
            }

            if (CheckLimits())
            {
                m_Stopped = true;
                return VALUE_NONE;
            }

            if (IsRoot)
            {
                RootMove& rootMove = *std::find(m_RootMoves.begin(), m_RootMoves.end(), move);
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
                bestMove = move;
                if (value > alpha)
                {
                    alpha = value;
                    if (IsPvNode)
                        UpdatePV(stack->PV, move, (stack + 1)->PV);
                }
            }
            if (value >= beta)
            {
                if (!position.IsCapture(move))
                {
                    if (move != stack->Killers[0] && move != stack->Killers[1])
                    {
                        stack->Killers[0] = stack->Killers[1];
                        stack->Killers[1] = move;
                    }
                    m_CounterMoves[GetFromSquare(previousMove)][GetToSquare(previousMove)] = move;
                }
                ttEntry->Update(ttHash, move, depth, BOUND_LOWER, GetValueForTT(value, stack->Ply));
                return value;
            }
        }

        if (bestValue == -VALUE_MATE)
        {
            if (inCheck)
                return MatedIn(stack->Ply);
            return VALUE_DRAW;
        }

        ttEntry->Update(ttHash, bestMove, depth, alpha > originalAlpha ? BOUND_EXACT : BOUND_UPPER, GetValueForTT(bestValue, stack->Ply));

        return bestValue;
    }

    template<Search::NodeType NT>
    inline ValueType Search::QuiescenceSearch(Position& position, SearchStack* stack, int depth, ValueType alpha, ValueType beta, bool cutNode)
    {
        constexpr bool IsPvNode = NT == PV;
        const bool inCheck = position.InCheck();
        Move pv[MAX_PLY];
        UndoInfo undo;

        stack->PositionHistory[0] = position.Hash;

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
        (stack + 1)->PositionHistory = stack->PositionHistory + 1;

        MoveSelector<QUIESCENCE> selector(position);

        ValueType bestValue = -VALUE_MATE;
        int moveIndex = 0;

        Move move;

        while ((move = selector.GetNextMove()) != MOVE_NONE)
        {
            STORM_ASSERT(position.InCheck() || position.SeeGE(move), "Invalid QMove");
            if (position.IsLegal(move))
            {
                moveIndex++;
                bool givesCheck = position.GivesCheck(move);
                Position movedPosition = position;
                movedPosition.ApplyMove(move, &undo, givesCheck);
                m_Nodes++;

                stack->CurrentMove = move;

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
                            UpdatePV(stack->PV, move, (stack + 1)->PV);
                    }
                }
                if (value >= beta)
                    return value;
            }
        }

        if (inCheck && bestValue == -VALUE_MATE)
            return MatedIn(stack->Ply);

        return alpha;
    }

    bool Search::IsDraw(const Position& position, SearchStack* stack) const
    {
        if (position.HalfTurnsSinceCaptureOrPush >= 100)
            return true;
        if (stack && stack->Ply + m_PositionHistory.size() >= 8)
        {
            int ply = 4;
            ZobristHash hash = position.Hash;
            int count = 0;
            while (true)
            {
                if (*(stack->PositionHistory - ply) == hash)
                {
                    count++;
                    if (count >= 1) // 2
                        return true;
                }
                else
                    break;
                ply += 4;
            }
        }
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
