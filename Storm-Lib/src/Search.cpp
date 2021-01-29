#include "Search.h"
#include <chrono>
#include <random>

namespace Storm
{

    int LmrReductions[MAX_PLY][MAX_MOVES];

    void InitSearch()
    {
        for (int depth = 0; depth < MAX_PLY; depth++)
        {
            for (int index = 0; index < MAX_MOVES; index++)
            {
                LmrReductions[depth][index] = int(1.0 + std::log(depth) + std::log(index) * 0.5);
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
        : m_TranspositionTable(ttSize), m_PositionHistory(), m_Settings(), m_TimeManager(), m_Book(nullptr), m_Log(log), m_Limits(), m_RootMoves(), m_Nodes(0), m_PvIndex(0), m_StartSearchTime(),
        m_Stopped(false), m_ShouldStop(false), m_SearchTables(std::make_unique<SearchTables>())
    {
        ClearTable<Move, SQUARE_MAX, SQUARE_MAX>(m_SearchTables->CounterMoves, MOVE_NONE);
        ClearTable<int16_t, COLOR_MAX, SQUARE_MAX, SQUARE_MAX>(m_SearchTables->History, 0);
        ClearTable<int16_t, P_LIMIT, SQUARE_MAX, P_LIMIT * SQUARE_MAX>(m_SearchTables->CounterMoveHistory, 0);
    }

    void Search::SetSettings(const SearchSettings& settings)
    {
        m_Settings = settings;
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

    void Search::Ponder(const Position& position, const std::function<void(const SearchResult&)>& callback)
    {
        Ponder(position, SearchLimits{}, callback);
    }

    void Search::Ponder(const Position& position, SearchLimits limits, const std::function<void(const SearchResult&)>& callback)
    {
        limits.Infinite = true;
        SearchBestMove(position, limits, callback);
    }

    BestMove Search::SearchBestMove(const Position& position, SearchLimits limits)
    {
        return SearchBestMove(position, limits, {});
    }

    BestMove Search::SearchBestMove(const Position& position, SearchLimits limits, const std::function<void(const SearchResult&)>& callback)
    {
        Position pos = position;
        m_Limits = limits;
        int depth = limits.Depth < 0 ? MAX_PLY : limits.Depth;

        if (!m_Limits.Infinite && m_Book != nullptr && pos.GetTotalHalfMoves() <= m_Book->GetCardinality() && m_Limits.Only.empty())
        {
            auto collection = m_Book->Probe(pos.Hash);
            if (collection)
            {
                BookEntry entry = collection->PickRandom();
                Move move = CreateMoveFromEntry(entry, pos);
                m_Limits.Only.insert(move);
            }
        }

        RootMove move = SearchRoot(pos, depth, callback);
        if (move.Pv.size() > 0)
            return { move.Pv[0], move.Pv.size() > 1 ? move.Pv[1] : MOVE_NONE };
        return { MOVE_NONE };
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

    RootMove Search::SearchRoot(Position& position, int depth, const std::function<void(const SearchResult&)>& callback)
    {
        m_RootMoves = GenerateRootMoves(position, m_Limits.Only);
        if (m_RootMoves.empty())
            return {};

        m_Stopped = false;
        m_ShouldStop = false;
        if (m_Limits.Milliseconds > 0 && !m_Limits.Infinite)
            m_TimeManager.SetMillisecondsToMove(m_Limits.Milliseconds);
        else
            m_TimeManager.Disable();
        m_TimeManager.StartSearch();
        int rootDepth = 1;

        Move pv[MAX_PLY];
        ZobristHash* positionHistory = new ZobristHash[m_PositionHistory.size() + MAX_PLY + 1];
        pv[0] = MOVE_NONE;
        SearchStack stack[MAX_PLY + 10];
        SearchStack* stackPtr = InitStack(stack, 4, position, pv, positionHistory);

        int skillLevelMultiPv = GetMultiPv(m_Settings.SkillLevel);
        int multiPv = std::max({ m_Settings.MultiPv, skillLevelMultiPv, 1 });
        multiPv = std::min(multiPv, int(m_RootMoves.size()));
        
        ValueType alpha = -VALUE_MATE;
        ValueType beta = VALUE_MATE;

        while (rootDepth <= depth)
        {
            for (RootMove& mv : m_RootMoves)
                mv.PreviousScore = mv.Score;
            for (int pvIndex = 0; pvIndex < multiPv; pvIndex++)
            {
                m_StartSearchTime = std::chrono::high_resolution_clock::now();
                m_PvIndex = pvIndex;
                m_Nodes = 0;
                int selDepth = 0;

                ValueType delta = 0;
                if (depth >= AspirationWindowDepth)
                {
                    delta = InitialAspirationWindow;
                    alpha = std::max(m_RootMoves[pvIndex].PreviousScore - delta, -VALUE_MATE);
                    beta = std::min(m_RootMoves[pvIndex].PreviousScore + delta, VALUE_MATE);
                }

                int betaCutoffs = 0;

                while (true)
                {
                    int adjustedDepth = std::max(1, rootDepth - betaCutoffs);
                    ValueType value = SearchPosition<PV>(position, stackPtr, adjustedDepth, alpha, beta, selDepth, false);

                    if (CheckLimits())
                    {
                        m_Stopped = true;
                        break;
                    }

                    std::stable_sort(m_RootMoves.begin() + pvIndex, m_RootMoves.end());

                    delta += delta / 2 + 2;

                    if (value <= alpha && value != -VALUE_MATE)
                    {
                        beta = (alpha + beta) / 2;
                        alpha = std::max(value - delta, -VALUE_MATE);
                        betaCutoffs = 0;
                    }
                    else if (value >= beta && value != VALUE_MATE)
                    {
                        beta = std::min(value + delta, VALUE_MATE);
                        betaCutoffs++;
                    }
                    else
                        break;
                }

                if (CheckLimits())
                {
                    m_Stopped = true;
                    break;
                }

                std::stable_sort(m_RootMoves.begin(), m_RootMoves.begin() + pvIndex + 1);

                RootMove& rootMove = m_RootMoves[pvIndex];

                // Don't report lines that are generated due to SkillLevel
                if (pvIndex < m_Settings.MultiPv)
                {
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
                        std::cout << " multipv " << (pvIndex + 1);
                        if (m_Limits.Only.size() == 1)
                            std::cout << " bookmove";
                        //if (hashFull >= 500)
                        //    std::cout << " hashfull " << hashFull;
                        std::cout << " pv";
                        for (Move move : rootMove.Pv)
                        {
                            std::cout << " " << UCI::FormatMove(move);
                        }
                        std::cout << std::endl;
                    }
                    if (callback)
                    {
                        SearchResult result;
                        result.BestMove = rootMove.Pv.size() > 0 ? rootMove.Pv[0] : MOVE_NONE;
                        result.PV = rootMove.Pv;
                        result.Score = rootMove.Score;
                        result.PVIndex = pvIndex;
                        result.Depth = rootDepth;
                        result.SelDepth = selDepth;
                        callback(result);
                    }
                }
            }

            if (CheckLimits())
            {
                m_Stopped = true;
                break;
            }

            rootDepth++;
        }

        // The previous score is guaranteed to be from a completed search
        for (RootMove& mv : m_RootMoves)
            mv.Score = mv.PreviousScore;
        std::stable_sort(m_RootMoves.begin(), m_RootMoves.end());

        delete[] positionHistory;
        if (m_RootMoves.empty())
            return {};
        return m_RootMoves[SelectBestMoveIndex(std::min(multiPv, skillLevelMultiPv), m_Settings.SkillLevel)];
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

        ValueType originalAlpha = alpha;

        if (stack->Ply >= MAX_PLY)
            return Evaluate(position);

        if (depth <= 0)
            return QuiescenceSearch<NT>(position, stack, depth, alpha, beta, cutNode);

        if (stack->Ply >= selDepth)
            selDepth = stack->Ply + 1;

        if (!IsRoot && IsDraw(position, stack))
            return VALUE_DRAW;

        // Mate distance pruning
        if (!IsRoot)
        {
            alpha = std::max(MatedIn(stack->Ply), alpha);
            beta = std::min(MateIn(stack->Ply), beta);
            if (alpha >= beta)
                return alpha;
        }

        SetCounterMoveHistoryPointer(cmhPtr, stack, &m_SearchTables->CounterMoveHistory, stack->Ply);

        Move previousMove = (stack - 1)->CurrentMove;
        Move move;

        bool ttHit = false;
        ZobristHash ttHash = position.Hash;
        TranspositionTableEntry* ttEntry = nullptr;
        if (stack->SkipMove != MOVE_NONE)
        {
            ttHash ^= stack->SkipMove;
            STORM_ASSERT(ttHash != position.Hash, "Invalid hash");
            STORM_ASSERT(m_TranspositionTable.HashToIndex(ttHash.Hash) != m_TranspositionTable.HashToIndex(position.Hash.Hash), "Invalid hash");
        }
        ttEntry = m_TranspositionTable.GetEntry(ttHash, ttHit);

        ValueType ttValue = IsRoot ? m_RootMoves[m_PvIndex].Score : ttHit ? GetValueFromTT(ttEntry->GetValue(), stack->Ply) : VALUE_NONE;
        Move ttMove = IsRoot ? m_RootMoves[m_PvIndex].Pv[0] : ttHit ? ttEntry->GetMove() : MOVE_NONE;

        if (!IsPvNode && ttHit && ttEntry->GetDepth() >= depth)
        {
            EntryBound bound = ttEntry->GetBound();
            if ((bound == BOUND_LOWER && ttValue >= beta) || (bound == BOUND_UPPER && ttValue <= alpha) || bound == BOUND_EXACT)
            {
                if (!position.IsCapture(ttMove))
                {
                    if (bound == BOUND_LOWER)
                    {
                        UpdateQuietStats(stack, ttMove);
                        AddToHistory(position, stack, cmhPtr, m_SearchTables.get(), ttMove, GetHistoryValue(depth));
                    }
                    else if (bound == BOUND_UPPER)
                        AddToHistory(position, stack, cmhPtr, m_SearchTables.get(), ttMove, -GetHistoryValue(depth));
                }
                return ttValue;
            }
        }

        // Static evaluation
        if (!inCheck)
        {
            if (ttHit && ttMove != MOVE_NONE)
            {
                stack->StaticEvaluation = ttValue;
            }
            else
            {
                stack->StaticEvaluation = Evaluate(position);
            }
        }

        const bool improving = !inCheck ? stack->StaticEvaluation >= (stack - 2)->StaticEvaluation : false;

        if (!IsPvNode && !inCheck && (stack - 1)->CurrentMove != MOVE_NONE && !IsMateScore(beta))
        {
            // Razoring
            if (depth <= RazorDepth && stack->SkipMove == MOVE_NONE && stack->StaticEvaluation + RazorMargin < beta)
            {
                ValueType value = QuiescenceSearch<NonPV>(position, stack, 0, alpha, beta, cutNode);
                if (value < beta)
                    return value;
            }

            // Futility Pruning
            if (position.GetNonPawnMaterial() > 0 && depth <= FutilityDepth && stack->StaticEvaluation - GetFutilityMargin(depth) >= beta)
            {
                return stack->StaticEvaluation;
            }

            // Null move pruning
            if (depth >= NullMoveDepth && (stack - 1)->CurrentMove != MOVE_NONE && stack->SkipMove == MOVE_NONE && stack->StaticEvaluation >= beta && position.GetNonPawnMaterial() >= 1 * RookValueEg && !IsMateScore(stack->StaticEvaluation))
            {
                Position movedPosition = position;
                movedPosition.ApplyNullMove();

                stack->CurrentMove = MOVE_NONE;
                stack->MoveCount = FirstMoveIndex;

                ValueType value = -SearchPosition<NonPV>(movedPosition, stack + 1, depth - GetNullMoveDepthReduction(depth, stack->StaticEvaluation, beta), -beta, -beta + 1, selDepth, true);
                if (value >= beta)
                    return IsMateScore(value) ? beta : value;
            }

            // ProbCut
            if (depth >= ProbCutDepth)
            {
                ValueType probCutBeta = GetProbCutBeta(beta, improving);
                if (ttHit && ttEntry->GetDepth() >= depth - 3 && ttValue >= probCutBeta && ttMove != MOVE_NONE && position.IsCapture(ttMove))
                    return probCutBeta;
                MoveSelector<QUIESCENCE> selector(position);

                UndoInfo undo;

                while ((move = selector.GetNextMove()) != MOVE_NONE)
                {
                    if (move != stack->SkipMove && position.IsLegal(move))
                    {
                        stack->CurrentMove = move;
                        (stack + 1)->Position = position;
                        (stack + 1)->Position.ApplyMove(move, &undo);

                        ValueType value = -QuiescenceSearch<NonPV>((stack + 1)->Position, stack + 1, 0, -probCutBeta, -probCutBeta + 1, !cutNode);
                        if (value >= probCutBeta)
                            value = -SearchPosition<NonPV>((stack + 1)->Position, stack + 1, depth - ProbCutDepth + 1, -probCutBeta, -probCutBeta + 1, selDepth, !cutNode);

                        if (value >= probCutBeta)
                        {
                            if (!(ttHit && ttEntry->GetDepth() >= depth - 3))
                            {
                                ttEntry->Update(ttHash, move, depth - 3, BOUND_LOWER, GetValueForTT(value, stack->Ply));
                            }
                            return value;
                        }
                    }
                }
            }
        }

        Move counterMove = m_SearchTables->CounterMoves[GetFromSquare(previousMove)][GetToSquare(previousMove)];
        MoveSelector<ALL_MOVES> selector(position, stack, ttMove, counterMove, stack->Killers, m_SearchTables.get());

        ValueType bestValue = -VALUE_MATE;
        Move bestMove = MOVE_NONE;
        int moveIndex = 0;

        while ((move = selector.GetNextMove()) != MOVE_NONE)
        {
            if (IsRoot && std::find(m_RootMoves.begin() + m_PvIndex, m_RootMoves.end(), move) == m_RootMoves.end())
                continue;

            if (move == stack->SkipMove)
                continue;

            const bool givesCheck = position.GivesCheck(move);
            const bool isCapture = position.IsCapture(move);
            const bool isPromotion = GetMoveType(move) == PROMOTION;
            const bool isCaptureOrPromotion = isCapture || isPromotion;

            // Shallow depth pruning
            if (!IsRoot && position.GetNonPawnMaterial(position.ColorToMove) > 0 && !IsMateScore(bestValue))
            {
                if (!isCaptureOrPromotion && !givesCheck)
                {
                    if (!position.SeeGE(move, -15 * (depth - 1) * (depth - 1)))
                        continue;
                }
                else if (selector.GetCurrentStage() == FIND_BAD_CAPTURES && !position.SeeGE(move, -PawnValueMg * depth))
                    continue;
            }

            // CMH Pruning
            if (!IsRoot && moveIndex >= FirstMoveIndex && depth <= CmhPruneDepth)
            {
                int piecePosition = position.GetPieceOnSquare(GetFromSquare(move)) * SQUARE_MAX + GetToSquare(move);
                if ((cmhPtr[0] == nullptr || cmhPtr[0][piecePosition] < 0) && (cmhPtr[1] == nullptr || cmhPtr[1][piecePosition] < 0))
                    continue;
            }

            if (!IsRoot && !position.IsLegal(move))
                continue;

            moveIndex++;
            stack->MoveCount = moveIndex;

            STORM_ASSERT(m_PvIndex != 0 || moveIndex != FirstMoveIndex || ttMove == MOVE_NONE || move == ttMove || ttMove == stack->SkipMove, "Invalid move ordering");

            int newDepth = depth - 1;
            int depthExtension = 0;

            // Singular extension
            if (!IsRoot && depth >= SingularExtensionDepth && stack->SkipMove == MOVE_NONE && move == ttMove && ttHit && !IsMateScore(ttValue) && ttEntry->GetBound() == BOUND_LOWER && ttEntry->GetDepth() >= depth - SingularDepthTolerance)
            {
                ValueType singularBeta = GetSingularBeta(ttValue, depth);
                stack->SkipMove = move;
                ValueType score = SearchPosition<NonPV>(position, stack, GetSingularDepth(depth), singularBeta - 1, singularBeta, selDepth, cutNode);
                stack->SkipMove = MOVE_NONE;
                if (score < singularBeta)
                    depthExtension = 1;
            }
            else if (GetMoveType(move) == CASTLE)
                depthExtension = 1;
            else if (givesCheck && position.SeeGE(move))
                depthExtension = 1;
            else if (!isCaptureOrPromotion)
            {
                // CMH Extension
                int piecePosition = position.GetPieceOnSquare(GetFromSquare(move)) * SQUARE_MAX + GetToSquare(move);
                if (cmhPtr[0] != nullptr && cmhPtr[1] != nullptr && cmhPtr[0][piecePosition] >= MAX_HISTORY_SCORE / 2 && cmhPtr[1][piecePosition] >= MAX_HISTORY_SCORE / 2)
                    depthExtension = 1;
            }

            newDepth += depthExtension;

            (stack + 1)->Position = position;
            (stack + 1)->Position.ApplyMove(move, &undo, givesCheck);
            m_Nodes++;

            stack->CurrentMove = move;

            ValueType value;
            bool fullDepthSearch = false;

            if (depth >= LmrDepth && moveIndex >= LmrMoveIndex + 2 * IsRoot && (!isCaptureOrPromotion || cutNode) && !depthExtension)
            {
                int reduction = GetLmrReduction<IsPvNode>(improving, depth, moveIndex);
                if ((stack - 1)->MoveCount > 13)
                    reduction--;

                reduction -= 2 * GetHistoryScore(stack, position, cmhPtr, move, m_SearchTables.get()) / MAX_HISTORY_SCORE;

                int lmrDepth = std::clamp(newDepth - reduction, 1, newDepth);
                value = -SearchPosition<NonPV>((stack + 1)->Position, stack + 1, lmrDepth, -(alpha + 1), -alpha, selDepth, true);
                fullDepthSearch = value > alpha && lmrDepth != newDepth;
            }
            else
            {
                fullDepthSearch = !IsPvNode || moveIndex > FirstMoveIndex;
            }

            if (fullDepthSearch)
            {
                value = -SearchPosition<NonPV>((stack + 1)->Position, stack + 1, newDepth, -(alpha + 1), -alpha, selDepth, !cutNode);
            }
            if (IsPvNode && (moveIndex == FirstMoveIndex || (value > alpha && (IsRoot || value < beta))))
            {
                pv[0] = MOVE_NONE;
                (stack + 1)->PV = pv;
                value = -SearchPosition<PV>((stack + 1)->Position, stack + 1, newDepth, -beta, -alpha, selDepth, false);
            }

            if (CheckLimits())
            {
                m_Stopped = true;
                return VALUE_NONE;
            }

            if (IsRoot)
            {
                RootMove& rootMove = *std::find(m_RootMoves.begin() + m_PvIndex, m_RootMoves.end(), move);
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
                
                if (value > alpha)
                {
                    bestMove = move;
                    alpha = value;
                    if (IsPvNode)
                        UpdatePV(stack->PV, move, (stack + 1)->PV);

                    if (value >= beta)
                    {
                        if (!isCaptureOrPromotion)
                        {
                            int16_t historyScore = GetHistoryValue(depth + (value > beta + 80));
                            UpdateQuietStats(stack, move);
                            AddToHistory(position, stack, cmhPtr, m_SearchTables.get(), move, historyScore);

                            for (int i = 0; i < quietMoveCount; i++)
                                AddToHistory(position, stack, cmhPtr, m_SearchTables.get(), quietMoves[i], -historyScore);
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

        if (stack->SkipMove == MOVE_NONE && !(IsRoot && m_PvIndex != 0))
            ttEntry->Update(ttHash, bestMove, depth, bestValue >= beta ? BOUND_LOWER : ((IsPvNode && bestMove != MOVE_NONE) ? BOUND_EXACT : BOUND_UPPER), GetValueForTT(bestValue, stack->Ply));

        return bestValue;
    }

    template<Search::NodeType NT>
    inline ValueType Search::QuiescenceSearch(Position& position, SearchStack* stack, int depth, ValueType alpha, ValueType beta, bool cutNode)
    {
        STORM_ASSERT(alpha < beta, "Invalid bounds");
        STORM_ASSERT(stack->SkipMove == MOVE_NONE, "Invalid skip move");
        constexpr bool IsPvNode = NT == PV;
        const bool inCheck = position.InCheck();
        Move pv[MAX_PLY];
        UndoInfo undo;

        alpha = std::max(alpha, MatedIn(stack->Ply));
        beta = std::min(beta, MateIn(stack->Ply));
        if (alpha >= beta)
            return alpha;

        depth = inCheck ? -1 : 0;

        stack->PositionHistory[0] = position.Hash;

        if (stack->Ply >= MAX_PLY)
            return Evaluate(position);

        if (IsDraw(position, stack))
            return VALUE_DRAW;

        ZobristHash ttHash = position.Hash;
        if (stack->SkipMove != MOVE_NONE)
            ttHash ^= stack->SkipMove;
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
                stack->StaticEvaluation = ttValue;
            else
                stack->StaticEvaluation = Evaluate(position);
                
            if (stack->StaticEvaluation >= beta)
                return stack->StaticEvaluation;
            if (alpha < stack->StaticEvaluation)
                alpha = stack->StaticEvaluation;
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

            moveIndex++;
            bool givesCheck = position.GivesCheck(move);
            Position movedPosition = position;
            movedPosition.ApplyMove(move, &undo, givesCheck);
            m_Nodes++;

            stack->CurrentMove = move;

            pv[0] = MOVE_NONE;
            ValueType value = -QuiescenceSearch<NT>(movedPosition, stack + 1, 0, -beta, -alpha, cutNode);

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

        ttEntry->Update(ttHash, bestMove, depth, bestValue >= beta ? BOUND_LOWER : ((IsPvNode && bestMove != MOVE_NONE) ? BOUND_EXACT : BOUND_UPPER), GetValueForTT(bestValue, stack->Ply));
        return bestValue;
    }

    void Search::UpdateQuietStats(SearchStack* stack, Move move)
    {
        if (move != stack->Killers[0])
        {
            stack->Killers[1] = stack->Killers[0];
            stack->Killers[0] = move;
        }
        if ((stack - 1)->CurrentMove != MOVE_NONE)
            m_SearchTables->CounterMoves[GetFromSquare((stack - 1)->CurrentMove)][GetToSquare((stack - 1)->CurrentMove)] = move;
    }

    bool Search::IsDraw(const Position& position, SearchStack* stack) const
    {
        if (position.HalfTurnsSinceCaptureOrPush >= 100 || InsufficientMaterial(position))
            return true;
        if (stack && stack->Ply + m_PositionHistory.size() >= 8)
        {
            int ply = 2;
            ZobristHash hash = position.Hash;
            int count = 0;
            while (ply <= position.HalfTurnsSinceCaptureOrPush)
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

    bool Search::CheckLimits() const
    {
        if (m_Limits.Infinite)
            return m_ShouldStop || m_Stopped;
        if (m_Nodes & 2048)
        {
            if (m_TimeManager.IsSearchComplete())
                return true;
            if (m_Limits.Nodes > 0)
            {
                return m_Nodes >= m_Limits.Nodes;
            }
        }
        return m_ShouldStop || m_Stopped;
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
                mv.Score = VALUE_DRAW;
                mv.PreviousScore = VALUE_DRAW;
                mv.SelDepth = 0;
                result.push_back(mv);
            }
        }
        return result;
    }

    int Search::SelectBestMoveIndex(int multipv, int skillLevel) const
    {
        if (skillLevel == 20 || multipv <= 1)
            return 0;
        std::mt19937 engine;
        engine.seed(time(nullptr));

        ValueType bestScore = m_RootMoves[0].Score;
        ValueType delta = std::min(bestScore - m_RootMoves[size_t(multipv) - 1].Score, PawnValueEg);
        ValueType weakness = 120 - 2 * skillLevel;
        ValueType maxScore = -VALUE_MATE;

        int bestIndex = 0;
        for (int i = 0; i < multipv; i++)
        {
            std::uniform_int_distribution<ValueType> dist(0, weakness);
            int push = (weakness * int(bestScore - m_RootMoves[i].Score) + delta * dist(engine)) / 120;
            if (m_RootMoves[i].Score + push >= maxScore)
            {
                maxScore = m_RootMoves[i].Score + push;
                bestIndex = i;
            }
        }
        return bestIndex;
    }

    SearchStack* Search::InitStack(SearchStack* stack, int count, const Position& position, Move* pv, ZobristHash* history) const
    {
        SearchStack* stackPtr = stack;
        ZobristHash* historyPtr = history;

        for (int i = 0; i < m_PositionHistory.size(); i++)
        {
            *historyPtr++ = m_PositionHistory[i];
        }

        for (int i = 0; i < count; i++)
        {
            stackPtr->Ply = -count + i + 1;
            stackPtr->PV = pv;
            stackPtr->Killers[0] = stackPtr->Killers[1] = MOVE_NONE;
            stackPtr->CurrentMove = MOVE_NONE;
            stackPtr->StaticEvaluation = VALUE_NONE;
            stackPtr->PositionHistory = historyPtr;
            stackPtr->MoveCount = 0;
            stackPtr->SkipMove = MOVE_NONE;
            stackPtr->Position = position;
            stackPtr++;
        }
        stackPtr->Killers[0] = stackPtr->Killers[1] = MOVE_NONE;
        return stackPtr - 1;
    }

}
