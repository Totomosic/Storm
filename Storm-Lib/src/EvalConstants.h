#pragma once
#include "Types.h"
#include "Bitboard.h"

#ifdef SWIG
#define STORM_API
#define ValueType int16_t
#endif

namespace Storm
{

    constexpr BitBoard QueensideMask = FILE_A_BB | FILE_B_BB | FILE_C_BB | FILE_D_BB;
    constexpr BitBoard KingsideMask = FILE_E_BB | FILE_F_BB | FILE_G_BB | FILE_H_BB;

    // Returns rank relative to WHITE
    template<Color C>
    constexpr Rank RelativeRank(Rank rank)
    {
        return C == COLOR_WHITE ? rank : Rank(RANK_MAX - rank - 1);
    }

    template<Color C>
    constexpr Rank RelativeRankBlack(Rank rank)
    {
        return C == COLOR_WHITE ? Rank(RANK_MAX - rank - 1) : rank;
    }

    template<Color C>
    constexpr SquareIndex RelativeSquare(SquareIndex square)
    {
        return C == COLOR_WHITE ? square : OppositeSquare(square);
    }

    STORM_API enum GameStage
    {
        MIDGAME,
        ENDGAME,
        GAME_STAGE_MAX,
    };

    // =======================================================================================================================================================================================
    // GAME STAGE
    // =======================================================================================================================================================================================

    constexpr int PawnStageWeight = 0;
    constexpr int KnightStageWeight = 6;
    constexpr int BishopStageWeight = 6;
    constexpr int RookStageWeight = 13;
    constexpr int QueenStageWeight = 28;
    constexpr int KingStageWeight = 0;

    constexpr int GameStageWeights[PIECE_COUNT] = {
      PawnStageWeight,
      KnightStageWeight,
      BishopStageWeight,
      RookStageWeight,
      QueenStageWeight,
      KingStageWeight,
    };

    constexpr int GameStageMax = 16 * PawnStageWeight + 4 * KnightStageWeight + 4 * BishopStageWeight +
                                 4 * RookStageWeight + 2 * QueenStageWeight + 2 * KingStageWeight;

    constexpr ValueType VALUE_MATE = 20000;
    constexpr ValueType VALUE_NONE = -VALUE_MATE - 100;
    constexpr ValueType VALUE_DRAW = 0;

    constexpr int MAX_PLY = 100;

    constexpr ValueType MateIn(int ply)
    {
        return VALUE_MATE - ply;
    }
    constexpr ValueType MatedIn(int ply)
    {
        return -VALUE_MATE + ply;
    }
    constexpr int GetPliesFromMateScore(ValueType score)
    {
        return score < 0 ? score + VALUE_MATE : VALUE_MATE - score;
    }
    constexpr bool IsMateScore(ValueType score)
    {
        return score >= MateIn(MAX_PLY) || score <= MatedIn(MAX_PLY);
    }

    // =======================================================================================================================================================================================
    // INITIATIVE
    // =======================================================================================================================================================================================

    constexpr ValueType InitiativeBonuses[4] = {
      4,
      66,
      71,
      89,
    };

    constexpr ValueType Tempo = 10;

    // =======================================================================================================================================================================================
    // MATERIAL
    // =======================================================================================================================================================================================

    constexpr ValueType PawnValueMg = 100;
    constexpr ValueType KnightValueMg = 310;
    constexpr ValueType BishopValueMg = 330;
    constexpr ValueType RookValueMg = 500;
    constexpr ValueType QueenValueMg = 1000;
    constexpr ValueType KingValueMg = 20000;

    constexpr ValueType PawnValueEg = 140;
    constexpr ValueType KnightValueEg = 310;
    constexpr ValueType BishopValueEg = 330;
    constexpr ValueType RookValueEg = 500;
    constexpr ValueType QueenValueEg = 1000;
    constexpr ValueType KingValueEg = 20000;

    constexpr ValueType PIECE_VALUES_MG[PIECE_COUNT + 1] = {
      0,   // PIECE_NONE
      PawnValueMg,
      KnightValueMg,
      BishopValueMg,
      RookValueMg,
      QueenValueMg,
      KingValueMg,
    };

    constexpr ValueType PIECE_VALUES_EG[PIECE_COUNT + 1] = {
      0,   // PIECE_NONE
      PawnValueEg,
      KnightValueEg,
      BishopValueEg,
      RookValueEg,
      QueenValueEg,
      KingValueEg,
    };

    constexpr ValueType GetPieceValueMg(Piece piece)
    {
        return PIECE_VALUES_MG[piece];
    }

    constexpr ValueType GetPieceValueEg(Piece piece)
    {
        return PIECE_VALUES_EG[piece];
    }

    // =======================================================================================================================================================================================
    // PIECE SQUARE TABLES
    // =======================================================================================================================================================================================

    /*
        Piece square tables defined in Centipawns from White's perspective
        Black tables are an exact mirror of the tables
        NOTE: Tables are REVERSED so that when viewing them they look viewing the board from White's perpective
        ie. the bottom left value corresponds to the square a1 however in terms of the array index it is h1
    */

    // clang-format off
    constexpr ValueType PieceSquareTables[GAME_STAGE_MAX][PIECE_COUNT][SQUARE_MAX] = {
        // MIDGAME
        {
            // PIECE_PAWN
            {
                0,   0,   0,   0,   0,   0,   0,   0,
               14,  16,  -4,  23,  36,  34,  -5, -27,
              -14,  -2,  20,  19,  67,  42,  10,  -8,
              -16,  -3, -12,  -2,  -2,  -3,  -4, -16,
              -22, -27,  -9, -18, -10, -10, -22, -31,
              -31, -25, -30, -36, -23, -27, -11, -30,
              -31, -18, -24, -24, -24,  -4,  13, -27,
                0,   0,   0,   0,   0,   0,   0,   0
            },
            // PIECE_KNIGHT
            {
             -136, -88, -46, -25,  19,-119,-112, -84,
                6,  19,  44,  66,  40,  68,  34,  31,
               17,  38,  53,  60,  82,  90,  53,  33,
               51,  58,  63,  75,  65,  74,  60,  70,
               54,  47,  65,  64,  75,  73,  76,  65,
               30,  43,  41,  50,  55,  57,  60,  39,
               19,  26,  34,  43,  50,  41,  44,  36,
               -5,  34,  18,  37,  45,  31,  34,  10
            },
            // PIECE_BISHOP
            {
               41,   2, -65, -64, -57, -84, -19,   7,
               -1,  10,  14,   0,  20,   8,  -8, -24,
               15,  19,  27,  45,  56,  57,  28,  33,
               20,  49,  34,  52,  47,  49,  57,  21,
               31,  33,  41,  54,  62,  47,  46,  57,
               33,  48,  38,  40,  40,  47,  49,  38,
               44,  39,  45,  27,  35,  46,  57,  40,
               32,  42,  29,  18,  29,  21,  17,  41
            },
            // PIECE_ROOK
            {
               15,  -2, -17,  12,   2, -30,  -1,  17,
                7, -11,  12,  31,   1,  11, -27,  10,
               -3,  18,   6,  19,  40,  36,  22,  -7,
               -6,   2,  16,  15,  30,  31,   7,   4,
              -11, -11,  -3,   9,  11,  10,  21, -14,
              -15,  -1,  -4,   5,   5,   6,  13, -12,
              -10,  -7,   1,   8,  12,   5,   2, -46,
                5,   9,  13,  21,  22,  20,  -6,   3
            },
            // PIECE_QUEEN
            {
              -30,  -4,  23,  17,  12,  29,  29,  15,
              -15,  -4,   6,  -7, -54, -14, -24,   8,
                9,  23,  25,  30,  23,  23,   7,  -2,
                2,  23,  23,  12,  34,  25,  32,  15,
               29,  19,  21,  14,  36,  26,  41,  28,
               15,  31,  19,  21,  24,  27,  42,  24,
               14,  24,  33,  28,  28,  41,  42,  14,
               21,  15,  18,  26,  15,   8,  -3,   8
            },
            // PIECE_KING
            {
                6,  19,  74, -24, -31, -54,  79,  94,
               13,  83,  41,  50,  28,  60, -25, -51,
              -22,  54, 141,  59,  79, 163,  69, -38,
              -50,  31,  69,  42,  50,  68,  25,-162,
              -57,  45,  33,  15,  11,   0,   3,-116,
              -69,  -2,  -4,  -3, -11,  -9,   6, -47,
              -13,  -6,  -5, -42, -33, -13,  16,   9,
               38,  58,  38,   2,   2,  38,  58,  38,
            }
        },
        // ENDGAME
        {
            // PIECE_PAWN
            {
                0,   0,   0,   0,   0,   0,   0,   0,
                7,  16,  27,  31,  37,  27,  32,  13,
                8,   6,   2,   5,  19,   3,  12,   7,
                2,  -6,  -8, -16, -12,  -3,  -2,  -4,
               -7,  -4, -19, -23, -19, -13,  -8, -15,
              -12, -10, -17, -12, -14, -10, -16, -20,
              -12,  -8, -10, -11,  -3,  -6, -10, -22,
                0,   0,   0,   0,   0,   0,   0,   0
            },
            // PIECE_KNIGHT
            {
              -47, -16,  -8, -18, -25, -27, -22, -70,
              -15,  -3, -17, -10,   0,  -8,  -7, -36,
               -5,  -1,  16,  15,  10,   4,   0, -21,
               -6,  -1,  17,  20,  14,  12,  -1, -14,
               -6,   6,  17,  18,  18,  17,   0,  -1,
              -14,  -6,   6,  14,  11,  -1, -12, -10,
              -26, -10, -14,   1,  -5, -17,  -6, -12,
              -23, -24, -15,  -6, -15, -18, -19, -40
            },
            // PIECE_BISHOP
            {
               -7,  -7, -10,  -4,  -9,   4,  -7, -18,
               -5,   1,  -2,  -2,  -2,  -7,  -6, -15,
               -7,   2,   4,   1,   4,   7,  -3, -11,
               -5,  -1,   2,  13,  11,   3,  -6, -14,
               -4,  -3,   8,  13,  10,   6,  -8, -18,
              -12,  -2,   1,   4,   3,   4,  -5,  -6,
              -10, -14, -13,  -4,  -7, -11, -15, -29,
              -14, -18, -10, -11, -12,  -4, -21, -22
            },
            // PIECE_ROOK
            {
               32,  39,  40,  37,  38,  39,  38,  38,
               29,  33,  34,  32,  28,  26,  32,  28,
               29,  28,  30,  27,  22,  27,  17,  23,
               31,  32,  30,  29,  23,  21,  23,  22,
               28,  31,  30,  26,  25,  21,  18,  20,
               25,  26,  28,  20,  20,  14,  11,  17,
               14,  20,  24,  19,  14,   9,   8,  14,
               22,  18,  23,  14,  17,  11,  19,  16
            },
            // PIECE_QUEEN
            {
               34,  41,  34,  34,  21,  39,  20,  23,
               51,  45,  56,  60,  68,  62,  36,  30,
               37,  45,  35,  53,  49,  23,  15,   2,
               59,  52,  59,  65,  53,  35,  41,  31,
               22,  49,  52,  62,  42,  56,  48,  42,
               24,  24,  45,  36,  45,  50,  37,  15,
               20,  13,   2,  19,  23,  -7, -13,  -3,
                4,   2,   1,  -8,  14,  -3,   6,  14
            },
            // PIECE_KING
            {
              -41, -25,  -4,   2,  -1, -19, -37, -32,
              -11,  19,  12,  11,  18,  20,  31, -11,
                2,  14,  27,  25,  26,  32,  23,   7,
               -1,  21,  22,  21,  18,  21,  21,   6,
               -3,  14,  19,  17,  16,  16,   9,  -2,
               -6,  10,  13,  12,  11,  15,   6,  -8,
              -21,  -1,   9,   7,   6,  11,   3, -17,
              -48, -20,  -6,  -4, -18,  -1, -15, -41
            }
        }
    };
    // clang-format on

    template<Color C, Piece P, GameStage S>
    constexpr ValueType GetPieceSquareValue(SquareIndex square)
    {
        return PieceSquareTables[S][P - PIECE_START][RelativeSquare<OtherColor(C)>(square)];
    }

    // =======================================================================================================================================================================================
    // MOBILITY + PIECES + THREATS
    // =======================================================================================================================================================================================

    constexpr ValueType MobilityWeights[PIECE_COUNT][GAME_STAGE_MAX] = {
      {0, 0},   // PIECE_PAWN
      {4, 4},   // PIECE_KNIGHT
      {3, 3},   // PIECE_BISHOP
      {2, 1},   // PIECE_ROOK
      {1, 2},   // PIECE_QUEEN
      {0, 0},   // PIECE_KING
    };

    constexpr int MobilityOffsets[PIECE_COUNT] = {
      0,    // PIECE_PAWN
      3,    // PIECE_KNIGHT
      6,    // PIECE_BISHOP
      6,    // PIECE_ROOK
      12,   // PIECE_QUEEN
      0,    // PIECE_KING
    };

    template<Piece P, GameStage S>
    constexpr ValueType GetMobilityBonus(int reachableSquares)
    {
        static_assert(P != PIECE_PAWN && P != PIECE_KING);
        return MobilityWeights[P - PIECE_START][S] * (reachableSquares - MobilityOffsets[P - PIECE_START]);
    }

    constexpr ValueType OutpostBonus[2] = {
      30,   // PIECE_KNIGHT
      20,   // PIECE_BISHOP
    };

    constexpr BitBoard CenterFiles = (FILE_C_BB | FILE_D_BB | FILE_E_BB | FILE_F_BB);
    constexpr BitBoard OutpostZone[COLOR_MAX] = {
      CenterFiles & (RANK_4_BB | RANK_5_BB | RANK_6_BB),   // COLOR_WHITE
      CenterFiles&(RANK_5_BB | RANK_4_BB | RANK_3_BB),     // COLOR_BLACK
    };

    constexpr BitBoard Center = (FILE_D_BB | FILE_E_BB) & (RANK_4_BB | RANK_5_BB);

    constexpr ValueType MinorBehindPawnBonus = 9;
    constexpr ValueType BishopTargetingCenterBonus = 20;
    constexpr ValueType BishopPairBonus[GAME_STAGE_MAX] = {40, 42};

    constexpr ValueType RookOnOpenFileBonus[2][GAME_STAGE_MAX] = {
      {27, 5},   // OPEN_FILE
      {13, 2},   // SEMI_OPEN_FILE
    };

    constexpr ValueType QueenXRayed[GAME_STAGE_MAX] = {
      -25,
      -5,
    };

    // Threats[AttackingPiece][AttackedPiece][STAGE]
    constexpr ValueType Threats[3][PIECE_COUNT - 1][GAME_STAGE_MAX] = {
      //   PAWN       KNIGHT      BISHOP       ROOK       QUEEN
      {{-2, 16}, {-5, 3}, {26, 32}, {58, 18}, {28, 2}},   // PIECE_KNIGHT
      {{5, 12}, {23, 34}, {6, 21}, {46, 22}, {38, 35}},   // PIECE_BISHOP
      {{3, 16}, {21, 24}, {21, 33}, {6, 10}, {29, 19}},   // PIECE_ROOK
    };

    template<Piece Attacker, GameStage S>
    constexpr ValueType GetThreatBonus(Piece attacked)
    {
        static_assert(Attacker != PIECE_QUEEN && Attacker != PIECE_KING && Attacker != PIECE_PAWN);
        STORM_ASSERT(attacked != PIECE_KING, "Invalid attacked piece");
        return Threats[Attacker - PIECE_KNIGHT][attacked - PIECE_PAWN][S];
    }

    // =======================================================================================================================================================================================
    // PAWNS
    // =======================================================================================================================================================================================

    extern BitBoard PassedPawnMasks[COLOR_MAX][SQUARE_MAX];
    extern BitBoard SupportedPawnMasks[COLOR_MAX][SQUARE_MAX];
    constexpr ValueType SupportedPassedPawn[GAME_STAGE_MAX] = {35, 70};
    constexpr ValueType DoubledPawnPenalty[GAME_STAGE_MAX] = {8, 24};

    constexpr ValueType ThreatByProtectedPawn[GAME_STAGE_MAX] = {55, 45};
    constexpr ValueType ThreatByProtectedPushedPawn[GAME_STAGE_MAX] = {14, 13};

    constexpr ValueType PassedPawnWeights[RANK_MAX][GAME_STAGE_MAX] = {
      {0, 0},       // RANK 1
      {2, 9},       // RANK 2
      {6, 12},      // RANK 3
      {5, 15},      // RANK 4
      {28, 31},     // RANK 5
      {82, 84},     // RANK 6
      {135, 125},   // RANK 7
      {0, 0},       // RANK 8
    };

    template<Color C, GameStage S>
    constexpr ValueType GetPassedPawnValue(Rank rank)
    {
        return PassedPawnWeights[RelativeRank<C>(rank)][S];
    }

    template<Color C>
    inline bool IsPassedPawn(SquareIndex square, BitBoard enemyPawns)
    {
        return !(PassedPawnMasks[C][square] & enemyPawns);
    }

    template<Color C>
    inline bool IsSupportedPawn(SquareIndex square, BitBoard ourPawns)
    {
        return SupportedPawnMasks[C][square] & ourPawns;
    }

    // =======================================================================================================================================================================================
    // KING SAFETY
    // =======================================================================================================================================================================================

    constexpr int PawnAttackWeight = 2;
    constexpr int KnightAttackWeight = 1;
    constexpr int BishopAttackWeight = 1;
    constexpr int RookAttackWeight = 1;
    constexpr int QueenAttackWeight = 1;
    constexpr int KingAttackWeight = 0;
    constexpr int CheckThreatWeight = 1;
    constexpr int SafeCheckWeight = 3;
    constexpr int KingSquareAttackWeight = 2;

    constexpr int MaxAttackerCount = 8;
    constexpr int AttackerCountScaling[MaxAttackerCount] = {0, 3, 7, 12, 16, 18, 19, 20};

    constexpr int AttackWeights[PIECE_COUNT] = {
      PawnAttackWeight,
      KnightAttackWeight,
      BishopAttackWeight,
      RookAttackWeight,
      QueenAttackWeight,
      KingAttackWeight,
    };

    constexpr int GetAttackWeight(Piece piece)
    {
        return AttackWeights[piece - PIECE_START];
    }

    // clang-format off
    // RANK_1 used when there is no pawn or pawn behind king
    constexpr ValueType PawnShield[SQUARE_MAX] = {
       -2, -20,  -2, -10, -10,  -2, -20,  -2,
       22,  20,  22,  10,   8,  22,  20,  22,
       25,  15,  -5,   4,   7,   4,  15,  25,
        2,  -3,  -4,   3,   1,  -5,  -9,   7,
        4, -16,   6,  -3, -10,   1,  -9,  -3,
       36,  84,  53, -18, -34,  -9,  23,  -4,
       42,  47,  95,  37,  73,  36,   5, -29,
      -32, -34, -15,  -6,  -5, -11, -22, -29,
    };

    // PawnStorm[Unopposed][SQUARE]
    constexpr ValueType PawnStorm[2][SQUARE_MAX] = {
        // Opposed
        {
            0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,
          -38,  28, -15, -30, -30,  -7,  22, -38,
          -28, -10, -12, -14,   0,  -1, -16, -35,
          -11,   2,  -3,   1,   9,  -5,  -5,  -5,
           16,  34,   0,   4,  15,  -8,   8,   9,
           11,  27,   2,   2,   3,   6,   0,   7,
            0,   0,   0,   0,   0,   0,   0,   0
        },
        // Unopposed
        {
            0,   0,   0,   0,   0,   0,   0,   0,
           83,  10, -26,  17, -54, -45,  10,  83,
           48, -79, -82, -11, -58, -94, -79,  48,
          -36,  -6, -34, -22, -16, -18,  -6, -36,
           -5,   4, -16, -10,  -9,  -7,   4,  -5,
           21,  27,   6,   6,   8,   3,  27,  21,
           14,  11,   3,   4,  -3,  11,  11,  14,
            0,   0,   0,   0,   0,   0,   0,   0
        }
    };
    // clang-format on

    // =======================================================================================================================================================================================
    // SPACE
    // =======================================================================================================================================================================================

    constexpr ValueType GetSpaceValue(int piecesCount, int safeCount)
    {
        return safeCount * safeCount * piecesCount / 32;
    }

}
