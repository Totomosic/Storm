#include "SearchThread.h"
#include <random>
#include <thread>

namespace Storm
{

    int LmrReductions[MAX_PLY][MAX_MOVES];

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

    Search::Search()
        : m_TimeManager(), m_TranspositionTable(), m_Limits(), m_ShouldStop(false), m_Stopped(false), m_EnableLogging(true), m_Threads()
    {
    }

    void Search::PushPosition(const ZobristHash& hash)
    {
        m_PositionHistory.push_back(hash);
    }

    void Search::Ponder(const Position& position, SearchLimits limits, const std::function<void(const SearchResult&)>& callback)
    {
        limits.Infinite = true;
        SearchBestMove(position, limits, callback);
    }

    BestMove Search::SearchBestMove(const Position& position, SearchLimits limits, const std::function<void(const SearchResult&)>& callback)
    {
        StateInfo st;
        Position pos = position;
        pos.Reset(&st);
        m_Limits = limits;
        return BeginSearch(pos, limits.Depth > 0 && !limits.Infinite ? limits.Depth : MAX_PLY, callback);
    }

    size_t Search::Perft(const Position& pos, int depth)
    {
        StateInfo st;
        Position position = pos;
        UndoInfo undo;
        size_t total = 0;
        Move moves[MAX_MOVES];
        MoveList moveList(moves);
        moveList.FillLegal<ALL>(position);

        auto startTime = std::chrono::high_resolution_clock::now();

        for (Move move : moveList)
        {
            position.ApplyMove(move, st, &undo);
            size_t perft = PerftPosition(position, depth - 1);
            position.UndoMove(move, undo);

            total += perft;

            if (m_EnableLogging)
                std::cout << UCI::FormatMove(move) << ": " << perft << std::endl;
        }
        if (m_EnableLogging)
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

    void Search::Stop()
    {
        m_ShouldStop = true;
    }

    size_t Search::PerftPosition(Position& position, int depth)
    {
        if (depth <= 0)
            return 1;
        StateInfo st;
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
            Move move = *it++;
            position.ApplyMove(move, st, &undo);
            nodes += PerftPosition(position, depth - 1);
            position.UndoMove(move, undo);
        }
        return nodes;
    }

    BestMove Search::BeginSearch(Position& position, int depth, const SearchCallback& callback)
    {
        m_Stopped = false;
        m_ShouldStop = false;

        if (m_Settings.HashBytes > 0 && !m_TranspositionTable.IsSameSize(m_Settings.HashBytes))
            m_TranspositionTable.SetSize(m_Settings.HashBytes);

        InitTimeManagement(position);
        CreateAndInitializeThreads(position, m_Settings.Threads);

        std::vector<std::thread> threads;
        for (int i = 1; i < m_Threads.size(); i++)
        {
            threads.push_back(std::thread(std::bind(&Search::IterativeDeepeningSearch, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4), this, &m_Threads[i], depth, callback));
        }

        IterativeDeepeningSearch(&m_Threads[0], depth, callback);

        for (std::thread& thread : threads)
            thread.join();

        Move bestMove = MOVE_NONE;
        Move ponderMove = MOVE_NONE;
        Thread& mainThread = m_Threads[0];
        if (mainThread.RootMoves.size() >= 1)
        {
            int bestIndex = SelectBestMoveIndex(&mainThread, mainThread.RootMoves.size(), m_Settings.SkillLevel);
            bestMove = mainThread.RootMoves[bestIndex].Pv[0];
            if (mainThread.RootMoves[bestIndex].Pv.size() > 1)
                ponderMove = mainThread.RootMoves[bestIndex].Pv[1];
        }
        return { bestMove, ponderMove };
    }

    void Search::IterativeDeepeningSearch(Thread* thread, int maxDepth, const SearchCallback& callback)
    {
        SearchStack* stack = thread->Stack + Thread::StackOffset;

        int multipv = std::clamp(m_Settings.MultiPv, GetMultiPv(m_Settings.SkillLevel), std::min(20, (int)thread->RootMoves.size()));

        int depthSinceMoveChange = 0;
        float evalAverage = 0;

        while (thread->Depth <= maxDepth)
        {
            if (thread->IsMain())
                m_TimeManager.StartNewDepth();
            thread->BestMoveChanges /= 2;
            for (int i = 0; i < multipv; i++)
            {
                thread->PvIndex = i;
                thread->SelDepth = 0;
                ValueType value = AspirationWindowSearch(thread, stack, callback);

                if (ShouldStopSearch(thread))
                    break;
            }

            if (ShouldStopSearch(thread))
                break;

            for (RootMove& mv : thread->RootMoves)
                mv.PreviousScore = mv.Score;
            std::stable_sort(thread->RootMoves.begin(), thread->RootMoves.end());

            // If playing a match, break if only one legal move
            if (thread->RootMoves.size() == 1 && (m_Limits.WhiteTime > 0 || m_Limits.BlackTime > 0))
                break;

            if (thread->IsMain() && (m_Limits.WhiteTime > 0 || m_Limits.BlackTime > 0))
            {
                int64_t depthTime = m_TimeManager.ElapsedMs();
                if (m_TimeManager.RemainingAllocatedTime() < depthTime && !m_Limits.Infinite)
                    break;

                evalAverage += (thread->RootMoves[0].Score - evalAverage) * (thread->Depth < 8 ? 1.0f : 0.1f);
             
                if (thread->BestMoveChanges == 0)
                    depthSinceMoveChange++;
                else
                    depthSinceMoveChange = 0;

                double extension = 1.0 + std::max(0.0, (evalAverage - thread->RootMoves[0].Score) / 70.0);
                double instability = 1.0 + std::max(1.0, 2.0 - 10.0 / thread->Depth) * thread->BestMoveChanges;
                double reduction = 1.0 - std::min(0.75, depthSinceMoveChange * depthSinceMoveChange / 1400.0);
                double multiplier = instability * reduction * extension;
                if (m_TimeManager.GetMaxTime() < 60000)
                    multiplier = std::min(multiplier, 1.0);
                m_TimeManager.SetAllocatedTimeMultiplier(multiplier);
            }

            thread->Depth++;
        }
    }

    ValueType Search::AspirationWindowSearch(Thread* thread, SearchStack* stack, const SearchCallback& callback)
    {
        bool isMainThread = thread->ThreadIndex == 0;
        int depth = thread->Depth;
        ValueType value = thread->RootMoves[0].PreviousScore;
        ValueType delta = 0;
        ValueType alpha = -VALUE_MATE;
        ValueType beta = VALUE_MATE;
        if (depth >= AspirationWindowDepth)
        {
            delta = InitialAspirationWindow;
            alpha = std::max<ValueType>(value - delta, -VALUE_MATE);
            beta = std::min<ValueType>(value + delta, VALUE_MATE);
        }

        int betaCutoffs = 0;
        while (true)
        {
            int adjustedDepth = std::max(1, depth - betaCutoffs);
            value = AlphaBetaSearch<true>(thread, stack, adjustedDepth, alpha, beta, false);

            std::stable_sort(thread->RootMoves.begin() + thread->PvIndex, thread->RootMoves.end());

            if (ShouldStopSearch(thread))
                break;

            if (isMainThread && ((value > alpha && value < beta) || m_TimeManager.TotalElapsedMs() > 3000) && thread->PvIndex < m_Settings.MultiPv)
            {
                size_t nodes = GetTotalNodes();
                RootMove& bestMove = thread->RootMoves[thread->PvIndex];
                if (m_EnableLogging)
                {
                    std::cout << "info depth " << depth << " seldepth " << thread->SelDepth << " score ";
                    if (!IsMateScore(bestMove.Score))
                    {
                        std::cout << "cp " << bestMove.Score;
                    }
                    else
                    {
                        if (bestMove.Score > 0)
                            std::cout << "mate " << (GetPliesFromMateScore(bestMove.Score) / 2 + 1);
                        else
                            std::cout << "mate " << -(GetPliesFromMateScore(bestMove.Score) / 2);
                    }
                    if (bestMove.Score <= alpha)
                        std::cout << " upperbound";
                    else if (bestMove.Score >= beta)
                        std::cout << " lowerbound";
                    std::cout << " nodes " << nodes << " nps " << size_t(nodes * 1000 / (m_TimeManager.TotalElapsedMs() + 1));
                    std::cout << " time " << m_TimeManager.TotalElapsedMs() << " hashfull " << m_TranspositionTable.HashFull() << " multipv " << thread->PvIndex + 1;
                    std::cout << " pv";
                    for (Move mv : bestMove.Pv)
                        std::cout << " " << UCI::FormatMove(mv);
                    std::cout << std::endl;
                }
                if (callback)
                {
                    SearchResult result;
                    result.BestMove = bestMove.Pv[0];
                    result.Depth = depth;
                    result.SelDepth = thread->SelDepth;
                    result.PVIndex = thread->PvIndex;
                    result.Score = bestMove.Score;
                    result.PV = bestMove.Pv;
                    callback(result);
                }
            }

            if (value <= alpha && value != -VALUE_MATE)
            {
                beta = (alpha + beta) / 2;
                alpha = std::max<ValueType>(value - delta, -VALUE_MATE);
                betaCutoffs = 0;
            }
            else if (value >= beta && value != VALUE_MATE)
            {
                beta = std::min<ValueType>(value + delta, VALUE_MATE);
                betaCutoffs++;
            }
            else
                break;
            delta += delta / 4 + 5;
        }

        return value;
    }

    template<bool IsPvNode>
    ValueType Search::AlphaBetaSearch(Thread* thread, SearchStack* stack, int depth, ValueType alpha, ValueType beta, bool cutNode)
    {
        constexpr int FirstMoveIndex = 1;
        const bool IsRoot = IsPvNode && stack->Ply == 0;
        Position& position = thread->Position;

        STORM_ASSERT(IsPvNode || alpha == beta - 1, "Invalid alpha beta bounds");
        STORM_ASSERT(alpha < beta, "Invalid alpha beta bounds");
        STORM_ASSERT(!(IsPvNode && cutNode), "Invalid configuration");

        ValueType originalAlpha = alpha;

        if (!IsRoot && Is3Fold(position, stack))
            return GetDrawValue(thread);

        if (depth <= 0)
            return QuiescenceSearch<IsPvNode>(thread, stack, depth, alpha, beta, cutNode);

        if (stack->Ply >= MAX_PLY)
            return Evaluate(position);

        if (stack->Ply >= thread->SelDepth)
            thread->SelDepth = stack->Ply + 1;

        const bool inCheck = position.InCheck();

        StateInfo st;
        Move pv[MAX_PLY];
        Move quietMoves[MAX_MOVES];
        int quietMoveCount = 0;
        UndoInfo undo;
        int16_t* cmhPtr[MaxCmhPly];

        (stack + 1)->Ply = stack->Ply + 1;
        pv[0] = MOVE_NONE;
        (stack + 1)->PV = pv;
        (stack + 1)->StaticEvaluation = VALUE_NONE;
        (stack + 1)->PositionHistory = stack->PositionHistory + 1;
        (stack + 1)->SkipMove = MOVE_NONE;
        (stack + 2)->Killers[0] = (stack + 2)->Killers[1] = MOVE_NONE;

        stack->PositionHistory[0] = position.Hash;

        // Mate distance pruning
        if (!IsRoot)
        {
            if (IsDraw(position, stack))
            {
                return VALUE_DRAW;
            }

            alpha = std::max(MatedIn(stack->Ply), alpha);
            beta = std::min(MateIn(stack->Ply), beta);
            if (alpha >= beta)
                return alpha;
        }

        SetCounterMoveHistoryPointer(cmhPtr, stack, &thread->Tables.CounterMoveHistory, stack->Ply);

        Move previousMove = (stack - 1)->CurrentMove;
        Move move;

        // Transposition table

        bool ttHit;
        TranspositionTableEntry* ttEntry;
        ZobristHash ttHash = position.Hash;
        if (stack->SkipMove != MOVE_NONE)
        {
            ttHash ^= uint64_t(stack->SkipMove) << 32;
        }
        ttEntry = m_TranspositionTable.GetEntry(ttHash, ttHit);

        ValueType ttValue = IsRoot ? thread->RootMoves[thread->PvIndex].Score : ttHit ? GetValueFromTT(ttEntry->GetValue(), stack->Ply) : VALUE_NONE;
        Move ttMove = IsRoot ? thread->RootMoves[thread->PvIndex].Pv[0] : ttHit ? ttEntry->GetMove() : MOVE_NONE;

        if (!IsPvNode && ttHit && ttEntry->GetDepth() >= depth)
        {
            EntryBound bound = ttEntry->GetBound();
            STORM_ASSERT(ttEntry->GetMove() != MOVE_NONE || bound == BOUND_UPPER, "Invalid Entry");
            if ((bound == BOUND_LOWER && ttValue >= beta) || (bound == BOUND_UPPER && ttValue <= alpha) || bound == BOUND_EXACT)
            {
                if (!position.IsCapture(ttMove))
                {
                    if (bound == BOUND_LOWER)
                    {
                        UpdateQuietStats(thread, stack, ttMove);
                        AddToHistory(position, stack, cmhPtr, &thread->Tables, ttMove, GetHistoryValue(depth));
                    }
                    else if (bound == BOUND_UPPER)
                        AddToHistory(position, stack, cmhPtr, &thread->Tables, ttMove, -GetHistoryValue(depth));
                }
                return ttValue;
            }
        }

        // Static evaluation
        if (!inCheck)
        {
            if (ttHit)
                stack->StaticEvaluation = ttEntry->GetStaticEvaluation();
            else if ((stack - 1)->CurrentMove == MOVE_NONE && (stack - 1)->StaticEvaluation > VALUE_NONE)
                stack->StaticEvaluation = -(stack - 1)->StaticEvaluation;
            else
                stack->StaticEvaluation = Evaluate(position);
            if (stack->StaticEvaluation == VALUE_DRAW)
                stack->StaticEvaluation = GetDrawValue(thread);
        }

        const bool improving = !inCheck ? stack->StaticEvaluation >= (stack - 2)->StaticEvaluation : false;

        if (!IsPvNode && !inCheck && (stack - 1)->CurrentMove != MOVE_NONE && !IsMateScore(beta))
        {
            // Futility Pruning
            if (position.GetNonPawnMaterial() > 0 && depth <= FutilityDepth && stack->StaticEvaluation - GetFutilityMargin(depth) >= beta)
            {
                return stack->StaticEvaluation;
            }

            // Razoring
            if (depth <= RazorDepth && stack->SkipMove == MOVE_NONE && stack->StaticEvaluation + RazorMargin < beta)
            {
                ValueType value = QuiescenceSearch<false>(thread, stack, 0, alpha, beta, cutNode);
                if (value < beta)
                    return value;
            }

            // Null move pruning
            if (depth >= NullMoveDepth && (stack - 1)->CurrentMove != MOVE_NONE && stack->SkipMove == MOVE_NONE && stack->StaticEvaluation >= beta && position.GetNonPawnMaterial(position.ColorToMove) > 0)
            {
                UndoInfo undo;
                position.ApplyNullMove(st, &undo);

                (stack + 1)->Position = position;
                stack->CurrentMove = MOVE_NONE;
                stack->MoveCount = FirstMoveIndex;

                ValueType value = -AlphaBetaSearch<false>(thread, stack + 1, depth - GetNullMoveDepthReduction(depth, stack->StaticEvaluation, beta), -beta, -beta + 1, true);
                position.UndoNullMove(undo);
                if (value >= beta)
                    return IsMateScore(value) ? beta : value;
            }

            // ProbCut
            if (depth >= ProbCutDepth)
            {
                ValueType probCutBeta = GetProbCutBeta(beta, improving);
                MoveSelector<QUIESCENCE> selector(position);

                UndoInfo undo;

                while ((move = selector.GetNextMove()) != MOVE_NONE)
                {
                    if (move != stack->SkipMove && position.IsLegal(move))
                    {
                        stack->CurrentMove = move;
                        position.ApplyMove(move, st, &undo);
                        (stack + 1)->Position = position;
                        ValueType value = -QuiescenceSearch<false>(thread, stack + 1, 0, -probCutBeta, -probCutBeta + 1, !cutNode);
                        if (value >= probCutBeta)
                        {
                            value = -AlphaBetaSearch<false>(thread, stack + 1, depth - ProbCutDepth + 1, -probCutBeta, -probCutBeta + 1, !cutNode);
                        }

                        position.UndoMove(move, undo);

                        if (value >= probCutBeta)
                        {
                            return value;
                        }
                    }
                }
            }
        }

        if (!inCheck)
        {
            if (IsPvNode && depth >= 6 && ttMove == MOVE_NONE)
                depth -= 2;
            if (cutNode && depth >= 9 && ttMove == MOVE_NONE)
                depth -= 1;
        }

        Move counterMove = thread->Tables.CounterMoves[GetFromSquare(previousMove)][GetToSquare(previousMove)];
        MoveSelector<ALL_MOVES> selector(position, stack, ttMove, counterMove, stack->Killers, &thread->Tables);

        ValueType bestValue = -VALUE_MATE;
        Move bestMove = MOVE_NONE;
        int moveIndex = 0;

        while ((move = selector.GetNextMove()) != MOVE_NONE)
        {
            if (IsRoot && std::find(thread->RootMoves.begin() + thread->PvIndex, thread->RootMoves.end(), move) == thread->RootMoves.end())
                continue;

            if (move == stack->SkipMove)
                continue;

            if (!IsRoot && !position.IsLegal(move))
                continue;

            if (thread->IsMain() && IsRoot && m_TimeManager.TotalElapsedMs() > 3000 && m_EnableLogging)
                std::cout << "info depth " << depth << " currmove " << UCI::FormatMove(move) << " currmovenumber " << moveIndex + 1 << std::endl;

            if (IsPvNode)
                (stack + 1)->PV = nullptr;

            const bool givesCheck = position.GivesCheck(move);
            const bool isCapture = position.IsCapture(move);
            const bool isPromotion = GetMoveType(move) == PROMOTION;
            const bool isCaptureOrPromotion = isCapture || isPromotion;

            // Shallow depth pruning
            if (!IsRoot && position.GetNonPawnMaterial(position.ColorToMove) > 0 && bestValue > MatedIn(MAX_PLY))
            {
                if (isCaptureOrPromotion || givesCheck)
                {
                    if (!position.SeeGE(move, -PawnValueEg * depth))
                        continue;
                }
                else if (!position.SeeGE(move, -PawnValueMg * depth))
                    continue;
            }

            // CMH Pruning
            if (!IsRoot && moveIndex > FirstMoveIndex && depth <= CmhPruneDepth)
            {
                int piecePosition = position.GetPieceOnSquare(GetFromSquare(move)) * SQUARE_MAX + GetToSquare(move);
                if ((cmhPtr[0] == nullptr || cmhPtr[0][piecePosition] < 0) && (cmhPtr[1] == nullptr || cmhPtr[1][piecePosition] < 0))
                    continue;
            }

            ++moveIndex;
            stack->MoveCount = moveIndex;

            int newDepth = depth - 1;
            int depthExtension = 0;

            // Singular extension
            if (!IsRoot && depth >= SingularExtensionDepth && stack->SkipMove == MOVE_NONE && move == ttMove && ttHit && !IsMateScore(ttValue) && ttEntry->GetBound() == BOUND_LOWER && ttEntry->GetDepth() >= depth - SingularDepthTolerance)
            {
                ValueType singularBeta = GetSingularBeta(ttValue, depth);
                stack->SkipMove = move;
                ValueType score = AlphaBetaSearch<false>(thread, stack, GetSingularDepth(depth), singularBeta - 1, singularBeta, cutNode);
                stack->SkipMove = MOVE_NONE;
                if (score < singularBeta)
                    depthExtension = 1;
                else if (singularBeta >= beta)
                    return singularBeta;
                else if (ttValue >= beta)
                    depthExtension = -2;
            }
            else if (givesCheck && depth >= 7 && std::abs(stack->StaticEvaluation) > PawnValueMg)
                depthExtension = 1;
            else if (!isCaptureOrPromotion)
            {
                // CMH Extension
                int piecePosition = position.GetPieceOnSquare(GetFromSquare(move)) * SQUARE_MAX + GetToSquare(move);
                if (cmhPtr[0] != nullptr && cmhPtr[1] != nullptr && cmhPtr[0][piecePosition] >= MAX_HISTORY_SCORE / 2 && cmhPtr[1][piecePosition] >= MAX_HISTORY_SCORE / 2)
                {
                    depthExtension = 1;
                }
            }

            newDepth += depthExtension;

            size_t preHash = position.Hash.Hash;

            position.ApplyMove(move, st, &undo, givesCheck);
            (stack + 1)->Position = position;
            ++thread->Nodes;

            size_t postHash = position.Hash.Hash;

            stack->CurrentMove = move;

            ValueType value;
            bool fullDepthSearch = false;

            if (depth >= LmrDepth && moveIndex >= LmrMoveIndex + 2 * IsRoot && (!isCaptureOrPromotion || cutNode) && !depthExtension)
            {
                int reduction = GetLmrReduction<IsPvNode>(improving, depth, moveIndex);
                if ((stack - 1)->MoveCount > 13)
                    reduction--;

                if (cutNode && move != stack->Killers[0])
                    reduction += 2;

                if (cutNode && move != stack->Killers[0])
                    reduction += 2;

                reduction -= 2 * GetHistoryScore(stack, stack->Position, cmhPtr, move, &thread->Tables) / MAX_HISTORY_SCORE;

                int lmrDepth = std::clamp(newDepth - reduction, 1, newDepth);
                value = -AlphaBetaSearch<false>(thread, stack + 1, lmrDepth, -(alpha + 1), -alpha, true);
                STORM_ASSERT(position.Hash.Hash == postHash, "Invalid");
                fullDepthSearch = value > alpha && lmrDepth != newDepth;
            }
            else
            {
                fullDepthSearch = !IsPvNode || moveIndex > FirstMoveIndex;
            }

            if (fullDepthSearch)
            {
                value = -AlphaBetaSearch<false>(thread, stack + 1, newDepth, -(alpha + 1), -alpha, !cutNode);
                STORM_ASSERT(position.Hash.Hash == postHash, "Invalid");
            }
            if (IsPvNode && (moveIndex == FirstMoveIndex || (value > alpha && (IsRoot || value < beta))))
            {
                pv[0] = MOVE_NONE;
                (stack + 1)->PV = pv;

                value = -AlphaBetaSearch<true>(thread, stack + 1, newDepth, -beta, -alpha, false);
                STORM_ASSERT(position.Hash.Hash == postHash, "Invalid");
            }

            position.UndoMove(move, undo);
            STORM_ASSERT(position.Hash.Hash == preHash, "Invalid {} {}", GetFENFromPosition(position), UCI::FormatMove(move));

            if (ShouldStopSearch(thread))
            {
                m_Stopped = true;
                return VALUE_NONE;
            }

            if (IsRoot)
            {
                RootMove& rootMove = *std::find(thread->RootMoves.begin(), thread->RootMoves.end(), move);
                if (moveIndex == FirstMoveIndex || (value > alpha))
                {
                    rootMove.Score = value;
                    rootMove.SelDepth = 0;
                    rootMove.Pv.resize(1);
                    for (Move* m = (stack + 1)->PV; *m != MOVE_NONE; ++m)
                        rootMove.Pv.push_back(*m);
                    if (moveIndex > FirstMoveIndex)
                        thread->BestMoveChanges++;
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

                    if (value >= beta)
                    {
                        if (!isCaptureOrPromotion)
                        {
                            int16_t historyScore = GetHistoryValue(depth);
                            UpdateQuietStats(thread, stack, move);
                            AddToHistory(position, stack, cmhPtr, &thread->Tables, move, historyScore);

                            for (int i = 0; i < quietMoveCount; i++)
                                AddToHistory(position, stack, cmhPtr, &thread->Tables, quietMoves[i], -historyScore);
                        }
                        break;
                    }
                }
            }

            if (!isCaptureOrPromotion)
                quietMoves[quietMoveCount++] = move;
        }

        if (bestValue == -VALUE_MATE)
        {
            if (stack->SkipMove != MOVE_NONE)
                return alpha;
            if (inCheck)
                return MatedIn(stack->Ply);
            return VALUE_DRAW;
        }

        if (stack->SkipMove == MOVE_NONE)
            ttEntry->Update(ttHash, bestMove, depth, bestValue >= beta ? BOUND_LOWER : ((IsPvNode && bestValue >= alpha) ? BOUND_EXACT : BOUND_UPPER), GetValueForTT(bestValue, stack->Ply), stack->StaticEvaluation);
        return bestValue;
    }

    template<bool IsPvNode>
    ValueType Search::QuiescenceSearch(Thread* thread, SearchStack* stack, int depth, ValueType alpha, ValueType beta, bool cutNode)
    {
        STORM_ASSERT(alpha < beta, "Invalid bounds");
        STORM_ASSERT(stack->SkipMove == MOVE_NONE, "Invalid skip move");
        Position& position = thread->Position;
        const bool inCheck = position.InCheck();

        ValueType futilityBase = -VALUE_MATE;

        StateInfo st;
        Move pv[MAX_PLY];
        UndoInfo undo;

        alpha = std::max(alpha, MatedIn(stack->Ply));
        beta = std::min(beta, MateIn(stack->Ply));
        if (alpha >= beta)
            return alpha;

        depth = inCheck ? -1 : 0;

        stack->PositionHistory[0] = position.Hash;

        if (IsDraw(position, stack))
            return VALUE_DRAW;

        if (stack->Ply >= MAX_PLY)
            return Evaluate(position);

        ZobristHash ttHash = position.Hash;
        bool ttHit;
        TranspositionTableEntry* ttEntry = m_TranspositionTable.GetEntry(ttHash, ttHit);
        Move ttMove = ttHit ? ttEntry->GetMove() : MOVE_NONE;
        ValueType ttValue = ttHit ? GetValueFromTT(ttEntry->GetValue(), stack->Ply) : VALUE_NONE;

        if (!IsPvNode && ttHit && ttEntry->GetDepth() >= depth && !IsMateScore(ttValue))
        {
            EntryBound bound = ttEntry->GetBound();
            if ((bound == BOUND_LOWER && ttValue >= beta) || (bound == BOUND_UPPER && ttValue <= alpha) || (bound == BOUND_EXACT))
                return ttValue;
        }

        if (!inCheck)
        {
            if (ttHit)
                stack->StaticEvaluation = ttEntry->GetStaticEvaluation();
            else
                stack->StaticEvaluation = Evaluate(position);

            if (stack->StaticEvaluation >= beta)
                return stack->StaticEvaluation;
            if (alpha < stack->StaticEvaluation)
                alpha = stack->StaticEvaluation;

            futilityBase = stack->StaticEvaluation + 80;
        }
        else
        {
            stack->StaticEvaluation = MatedIn(stack->Ply);
        }

        (stack + 1)->Ply = stack->Ply + 1;
        (stack + 1)->PV = pv;
        (stack + 1)->PositionHistory = stack->PositionHistory + 1;
        (stack + 1)->StaticEvaluation = VALUE_NONE;
        (stack + 1)->SkipMove = MOVE_NONE;

        MoveSelector<QUIESCENCE> selector(position);

        ValueType bestValue = stack->StaticEvaluation;
        Move bestMove = MOVE_NONE;
        int moveIndex = 0;

        Move move;

        while ((move = selector.GetNextMove()) != MOVE_NONE)
        {
            if (!position.IsLegal(move))
                continue;

            ++moveIndex;
            bool givesCheck = position.GivesCheck(move);

            if (bestValue > MatedIn(MAX_PLY) && !givesCheck && !inCheck && futilityBase > MatedIn(MAX_PLY) && GetMoveType(move) != PROMOTION)
            {
                if (moveIndex > 2)
                    continue;

                ValueType futilityValue = futilityBase + GetPieceValueEg(TypeOf(position.GetPieceOnSquare(GetToSquare(move))));
                if (futilityValue <= alpha)
                {
                    bestValue = std::max(bestValue, futilityValue);
                    continue;
                }

                if (futilityBase <= alpha && !position.SeeGE(move, 1))
                {
                    bestValue = std::max(bestValue, futilityBase);
                    continue;
                }
            }

            position.ApplyMove(move, st, &undo, givesCheck);
            ++thread->Nodes;
            stack->CurrentMove = move;
            pv[0] = MOVE_NONE;
            ValueType value = -QuiescenceSearch<IsPvNode>(thread, stack + 1, 0, -beta, -alpha, cutNode);
            position.UndoMove(move, undo);

            if (ShouldStopSearch(thread))
            {
                m_Stopped = true;
                return VALUE_NONE;
            }

            if (value > bestValue)
            {
                bestValue = value;
                if (value > alpha)
                {
                    bestMove = move;
                    alpha = value;
                    if (IsPvNode)
                        UpdatePV(stack->PV, move, (stack + 1)->PV);

                    if (value >= beta)
                        break;
                }
            }
        }

        if (inCheck && bestValue == -VALUE_MATE)
            return MatedIn(stack->Ply);

        ttEntry->Update(ttHash, bestMove, depth, bestValue >= beta ? BOUND_LOWER : ((IsPvNode && bestMove != MOVE_NONE) ? BOUND_EXACT : BOUND_UPPER), GetValueForTT(bestValue, stack->Ply), stack->StaticEvaluation);
        return bestValue;
    }

    void Search::UpdateQuietStats(Thread* thread, SearchStack* stack, Move move) const
    {
        if (move != stack->Killers[0])
        {
            stack->Killers[1] = stack->Killers[0];
            stack->Killers[0] = move;
        }
        if ((stack - 1)->CurrentMove != MOVE_NONE)
            thread->Tables.CounterMoves[GetFromSquare((stack - 1)->CurrentMove)][GetToSquare((stack - 1)->CurrentMove)] = move;
    }

    bool Search::Is3Fold(const Position& position, SearchStack* stack) const
    {
        size_t size = stack->Ply + m_PositionHistory.size();
        if (stack && stack->Ply + m_PositionHistory.size() >= 8)
        {
            int ply = 2;
            ZobristHash hash = position.Hash;
            int count = 0;
            while (ply <= position.HalfTurnsSinceCaptureOrPush && ply < size)
            {
                if (*(stack->PositionHistory - ply) == hash)
                {
                    count++;
                    if (count >= 1) // 2
                        return true;
                }
                ply += 2;
            }
        }
        return false;
    }

    bool Search::IsDraw(const Position& position, SearchStack* stack) const
    {
        if (position.HalfTurnsSinceCaptureOrPush >= 100 || InsufficientMaterial(position))
            return true;
        return false;
    }

    bool Search::ShouldStopSearch(Thread* thread) const
    {
        if (m_Limits.Infinite)
            return m_ShouldStop || m_Stopped;
        if (thread->Nodes & 2048)
        {
            if (m_TimeManager.IsSearchComplete())
                return true;
            if (m_Limits.Nodes > 0)
            {
                return thread->Nodes >= m_Limits.Nodes;
            }
        }
        return m_ShouldStop || m_Stopped;
    }

    int Search::SelectBestMoveIndex(Thread* thread, int multipv, int skillLevel) const
    {
        if (skillLevel == 20 || multipv <= 1)
            return 0;
        std::mt19937 engine;
        engine.seed(time(nullptr));

        ValueType bestScore = thread->RootMoves[0].Score;
        ValueType delta = std::min<ValueType>(bestScore - thread->RootMoves[size_t(multipv) - 1].Score, PawnValueEg);
        ValueType weakness = 120 - 2 * skillLevel;
        ValueType maxScore = -VALUE_MATE;

        int bestIndex = 0;
        for (int i = 0; i < multipv; i++)
        {
            std::uniform_int_distribution<ValueType> dist(0, weakness);
            int push = (weakness * int(bestScore - thread->RootMoves[i].Score) + delta * dist(engine)) / 120;
            if (thread->RootMoves[i].Score + push >= maxScore)
            {
                maxScore = thread->RootMoves[i].Score + push;
                bestIndex = i;
            }
        }
        return bestIndex;
    }

    void Search::InitTimeManagement(const Position& position)
    {
        if (m_Limits.Milliseconds > 0 && !m_Limits.Infinite)
        {
            m_TimeManager.SetOptimalTime(m_Limits.Milliseconds);
            m_TimeManager.SetMaxTime(m_Limits.Milliseconds);
        }
        else
        {
            int colorTime = position.ColorToMove == COLOR_WHITE ? m_Limits.WhiteTime : m_Limits.BlackTime;
            int colorIncrement = position.ColorToMove == COLOR_WHITE ? m_Limits.WhiteIncrement : m_Limits.BlackIncrement;
            int allocatedTime = 0;

            if (colorIncrement > 0)
                allocatedTime = int(colorTime * (1 + position.TotalTurns / 40.0f) / 16 + colorIncrement);
            if (m_Limits.MovesToGo > 0)
                allocatedTime = colorTime / (std::min(m_Limits.MovesToGo, 100) + 1) * 3 / 2;
            else
                allocatedTime = colorTime / 20;

            m_TimeManager.SetOptimalTime(allocatedTime);
            m_TimeManager.SetMaxTime(colorTime);
        }
        m_TimeManager.StartSearch();
    }

    void Search::CreateAndInitializeThreads(Position& position, int count)
    {
        m_Threads.resize(count);
        for (int i = 0; i < count; i++)
        {
            InitializeThread(position, &m_Threads[i], i);
        }
    }

    std::vector<RootMove> Search::GenerateRootMoves(const Position& position, const std::unordered_set<Move>& only) const
    {
        Move buffer[MAX_MOVES];
        MoveList list(buffer);
        list.FillLegal<ALL>(position);
        std::vector<RootMove> result;
        for (Move move : list)
        {
            if (only.empty() || only.find(move) != only.end())
            {
                RootMove mv;
                mv.Pv = { move };
                mv.Score = -VALUE_MATE;
                mv.PreviousScore = -VALUE_MATE;
                mv.SelDepth = 0;
                result.push_back(mv);
            }
        }
        return result;
    }

    void Search::InitializeThread(Position& position, Thread* thread, int index)
    {
        constexpr int count = Thread::StackOffset + 1;

        thread->PvBuffer[0] = MOVE_NONE;
        thread->Nodes = 0;
        thread->Depth = 1;
        thread->SelDepth = 0;
        thread->RootMoves = GenerateRootMoves(position, m_Limits.Only);
        thread->PositionHistory = std::make_unique<ZobristHash[]>(m_PositionHistory.size() + MAX_PLY + 1 + count);
        thread->Position = position;
        thread->Position.Reset(&thread->RootStateInfo);
        thread->PvIndex = 0;
        thread->ThreadIndex = index;
        if (!thread->Initialized)
        {
            ClearTable<Move, SQUARE_MAX, SQUARE_MAX>(thread->Tables.CounterMoves, MOVE_NONE);
            ClearTable<int16_t, COLOR_MAX, SQUARE_MAX, SQUARE_MAX>(thread->Tables.History, 0);
            ClearTable<int16_t, P_LIMIT, SQUARE_MAX, P_LIMIT* SQUARE_MAX>(thread->Tables.CounterMoveHistory, 0);
            thread->Initialized = true;
        }
        SearchStack* stackPtr = thread->Stack;
        ZobristHash* historyPtr = thread->PositionHistory.get() + std::max(0, (count - (int)m_PositionHistory.size()));

        for (int i = 0; i < m_PositionHistory.size(); i++)
        {
            *historyPtr++ = m_PositionHistory[i];
        }

        for (int i = 0; i < count; i++)
        {
            stackPtr->Ply = -count + i + 1;
            stackPtr->PV = thread->PvBuffer;
            stackPtr->Killers[0] = stackPtr->Killers[1] = MOVE_NONE;
            stackPtr->CurrentMove = MOVE_NONE;
            stackPtr->StaticEvaluation = VALUE_NONE;
            stackPtr->PositionHistory = historyPtr - count + i;
            stackPtr->MoveCount = 0;
            stackPtr->SkipMove = MOVE_NONE;
            stackPtr->Position = position;
            stackPtr++;
        }
        stackPtr->Killers[0] = stackPtr->Killers[1] = MOVE_NONE;
    }

}
