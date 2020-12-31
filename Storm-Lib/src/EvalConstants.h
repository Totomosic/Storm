#pragma once
#include "Types.h"
#include "Bitboard.h"

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
	constexpr int KnightStageWeight = 1;
	constexpr int BishopStageWeight = 1;
	constexpr int RookStageWeight = 3;
	constexpr int QueenStageWeight = 6;
	constexpr int KingStageWeight = 0;

	constexpr int GameStageWeights[PIECE_COUNT] = {
		PawnStageWeight,
		KnightStageWeight,
		BishopStageWeight,
		RookStageWeight,
		QueenStageWeight,
		KingStageWeight,
	};

	constexpr int GameStageMax = 16 * PawnStageWeight + 4 * KnightStageWeight + 4 * BishopStageWeight + 4 * RookStageWeight + 2 * QueenStageWeight + 2 * KingStageWeight;

	constexpr ValueType VALUE_MATE = 100000;
	constexpr ValueType VALUE_NONE = -VALUE_MATE - 100;
	constexpr ValueType VALUE_DRAW = 0;

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
	
	constexpr ValueType PawnValueMg		= 100;
	constexpr ValueType KnightValueMg	= 310;
	constexpr ValueType BishopValueMg	= 330;
	constexpr ValueType RookValueMg		= 500;
	constexpr ValueType QueenValueMg	= 950;
	constexpr ValueType KingValueMg		= 20000;

	constexpr ValueType PawnValueEg		= 140;
	constexpr ValueType KnightValueEg	= 310;
	constexpr ValueType BishopValueEg	= 330;
	constexpr ValueType RookValueEg		= 500;
	constexpr ValueType QueenValueEg	= 950;
	constexpr ValueType KingValueEg		= 20000;

	constexpr ValueType PIECE_VALUES_MG[PIECE_COUNT + 1] = {
		0, // PIECE_NONE
		PawnValueMg,
		KnightValueMg,
		BishopValueMg,
		RookValueMg,
		QueenValueMg,
		KingValueMg,
	};

	constexpr ValueType PIECE_VALUES_EG[PIECE_COUNT + 1] = {
		0, // PIECE_NONE
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

	// On InitEvaluation() this is populated by copying and mirroring the corresponding tables
	extern ValueType PieceSquareTables[COLOR_MAX][PIECE_COUNT][GAME_STAGE_MAX][SQUARE_MAX];

	constexpr ValueType WhitePawnsTableReversed[SQUARE_MAX] = {
		0,  0,  0,  0,  0,  0,  0,  0,
	   50, 50, 50, 50, 50, 50, 50, 50,
	   10, 10, 20, 20, 30, 20, 10, 10,
		5,  5, 10,  5, 10, 10,  5,  5,
		5,  0, 10, 15, 25,  0,  0,  5,
		5,  5,  5,  7, 15, 11,  5,  5,
		5, 10, 10,-10,-10, 10,  5,  5,
		0,  0,  0,  0,  0,  0,  0,  0,
	};

	constexpr ValueType WhiteKnightsTableReversed[SQUARE_MAX] = {
		-50,-40,-30,-30,-30,-30,-40,-50,
		-40,  0,  0,  0,  0,  0,  0,-40,
		-30,  0, 28, 25, 25, 28,  0,-30,
		-30,  5, 15, 20, 20, 15,  5,-30,
		-30,  0, 15, 20, 20, 15,  0,-30,
		-30,  5, 10, 15, 15, 10,  5,-30,
		-40,  0,  0,  5,  5,  0,  0,-40,
		-50,-40,-30,-30,-30,-30,-40,-50,
	};

	constexpr ValueType WhiteBishopsTableReversed[SQUARE_MAX] = {
		-20,-10,-10,-10,-10,-10,-10,-20,
		-10,  0,  0,  0,  0,  0,  0,-10,
		-10,  0,  5, 10, 10,  5,  0,-10,
		-10,  5,  5, 15, 15,  5,  5,-10,
		-10,  0, 10, 25, 25, 10,  0,-10,
		-10, 10, 10, 10, 10, 10, 10,-10,
		-10, 15, 10,  5,  5, 10, 15,-10,
		-20,-10,-10,-10,-10,-10,-10,-20,
	};

	constexpr ValueType WhiteRooksTableReversed[SQUARE_MAX] = {
		 0,  0,  0,  0,  0,  0,  0,  0,
		 5, 10, 10, 10, 10, 10, 10,  5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		 0,  0,  0,  5,  5,  0,  0,  0
	};

	constexpr ValueType WhiteQueensTableReversed[SQUARE_MAX] = {
		-20,-10,-10, -5, -5,-10,-10,-20,
		-10,  0,  0,  0,  0,  0,  0,-10,
		-10,  0,  5,  5,  5,  5,  0,-10,
		 -5,  0,  5,  5,  5,  5,  0, -5,
		  0,  0,  5,  5,  5,  5,  0, -5,
		-10,  5,  5,  5,  5,  5,  0,-10,
		-10,  0,  5,  0,  0,  0,  0,-10,
		-20,-10,-10, -5, -5,-10,-10,-20
	};

	constexpr ValueType WhiteKingsTableMidgameReversed[SQUARE_MAX] = {
		-30,-40,-40,-50,-50,-40,-40,-30,
		-30,-40,-40,-50,-50,-40,-40,-30,
		-30,-40,-40,-50,-50,-40,-40,-30,
		-30,-40,-40,-50,-50,-40,-40,-30,
		-20,-30,-30,-40,-40,-30,-30,-20,
		-10,-20,-20,-20,-20,-20,-20,-10,
		 20, 20,  0,  0,  0,  0, 20, 20,
		 20, 30, 10,  0,  0, 10, 30, 20
	};

	constexpr ValueType WhiteKingsTableEndgameReversed[SQUARE_MAX] = {
		-50,-40,-30,-20,-20,-30,-40,-50,
		-30,-20,-10,  0,  0,-10,-20,-30,
		-30,-10, 20, 30, 30, 20,-10,-30,
		-30,-10, 30, 40, 40, 30,-10,-30,
		-30,-10, 30, 40, 40, 30,-10,-30,
		-30,-10, 20, 30, 30, 20,-10,-30,
		-30,-30,  0,  0,  0,  0,-30,-30,
		-50,-30,-30,-30,-30,-30,-30,-50
	};

	// =======================================================================================================================================================================================
	// MOBILITY + PIECES + THREATS
	// =======================================================================================================================================================================================

	constexpr ValueType MobilityWeights[PIECE_COUNT][GAME_STAGE_MAX] = {
		{ 0, 0 }, // PIECE_PAWN
		{ 4, 4 }, // PIECE_KNIGHT
		{ 3, 3 }, // PIECE_BISHOP
		{ 2, 1 }, // PIECE_ROOK
		{ 1, 2 }, // PIECE_QUEEN
		{ 0, 0 }, // PIECE_KING
	};

	constexpr int MobilityOffsets[PIECE_COUNT] = {
		0,  // PIECE_PAWN
		3,  // PIECE_KNIGHT
		6,  // PIECE_BISHOP
		6,  // PIECE_ROOK
		12, // PIECE_QUEEN
		0,  // PIECE_KING
	};

	constexpr ValueType OutpostBonus[2] = {
		30, // PIECE_KNIGHT
		20, // PIECE_BISHOP
	};

	constexpr BitBoard CenterFiles = (FILE_C_BB | FILE_D_BB | FILE_E_BB | FILE_F_BB);
	constexpr BitBoard OutpostZone[COLOR_MAX] = {
		CenterFiles & (RANK_4_BB | RANK_5_BB | RANK_6_BB), // COLOR_WHITE
		CenterFiles & (RANK_5_BB | RANK_4_BB | RANK_3_BB), // COLOR_BLACK
	};

	constexpr BitBoard Center = (FILE_D_BB | FILE_E_BB) & (RANK_4_BB | RANK_5_BB);

	constexpr ValueType MinorBehindPawnBonus = 9;
	constexpr ValueType BishopTargettingCenterBonus = 20;

	constexpr ValueType RookOnOpenFileBonus[2][GAME_STAGE_MAX] = {
		{ 27, 5 }, // OPEN_FILE
		{ 13, 2 }, // SEMI_OPEN_FILE
	};

	constexpr ValueType QueenXRayed[GAME_STAGE_MAX] = {
		-25,
		-5,
	};

	// Threats[AttackingPiece][AttackedPiece][STAGE]
	constexpr ValueType Threats[PIECE_COUNT - 3][PIECE_COUNT - 1][GAME_STAGE_MAX] = {
		//   PAWN       KNIGHT      BISHOP       ROOK       QUEEN
		{ { -2, 16 }, { -5,  3 }, { 26, 32 }, { 58, 18 }, { 28,  2 } }, // PIECE_KNIGHT
		{ {  5, 12 }, { 23, 34 }, {  6, 21 }, { 46, 22 }, { 38, 35 } }, // PIECE_BISHOP
		{ {  3, 16 }, { 21, 24 }, { 21, 33 }, {  6, 10 }, { 29, 19 } }, // PIECE_ROOK
	};

	// =======================================================================================================================================================================================
	// PAWNS
	// =======================================================================================================================================================================================

	extern BitBoard PassedPawnMasks[COLOR_MAX][SQUARE_MAX];
	extern BitBoard SupportedPawnMasks[COLOR_MAX][SQUARE_MAX];
	constexpr ValueType SupportedPassedPawn[GAME_STAGE_MAX] = { 35, 70 };
	constexpr ValueType DoubledPawnPenalty[GAME_STAGE_MAX] = { 8, 24 };

	constexpr ValueType PassedPawnWeights[RANK_MAX][GAME_STAGE_MAX] = {
		{   0,   0 }, // RANK 1
		{   2,   9 }, // RANK 2
		{   6,  12 }, // RANK 3
		{   5,  15 }, // RANK 4
		{  28,  31 }, // RANK 5
		{  82,  84 }, // RANK 6
		{ 135, 125 }, // RANK 7
		{   0,   0 }, // RANK 8
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

	constexpr int MAX_ATTACK_UNITS = 100;
	constexpr ValueType KingSafetyTable[MAX_ATTACK_UNITS] = {
		0,   0,   1,   2,   3,   5,   7,   9,  12,  15,
		18,  22,  26,  30,  35,  39,  44,  50,  56,  62,
		68,  75,  82,  85,  89,  97, 105, 113, 122, 131,
		140, 150, 169, 180, 191, 202, 213, 225, 237, 248,
		260, 272, 283, 295, 307, 319, 330, 342, 354, 366,
		377, 389, 401, 412, 424, 436, 448, 459, 471, 483,
		494, 500, 500, 500, 500, 500, 500, 500, 500, 500,
		500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
		500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
		500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	};

	constexpr int PawnAttackWeight = 0;
	constexpr int KnightAttackWeight = 8;
	constexpr int BishopAttackWeight = 3;
	constexpr int RookAttackWeight = 3;
	constexpr int QueenAttackWeight = 5;
	constexpr int KingAttackWeight = 0;

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

	constexpr ValueType PawnShield[SQUARE_MAX] = {
		0,   0,   0,   0,   0,   0,   0,   0,
		0,  20,  22,  10,   8,  22,  12,   3,
	   11,  21,  -5,   4,   7,   4,   7,  13,
		2,  -3,  -4,   3,   1,  -5,  -9,   7,
		4, -16,   6,  -3, -10,   1,  -9,  -3,
	   36,  84,  53, -18, -34,  -9,  23,  -4,
	   42,  47,  95,  37,  73,  36,   5, -29,
	  -32, -34, -15,  -6,  -5, -11, -22, -29,
	};

	// PawnStorm[Unopposed][SQUARE]
	constexpr ValueType PawnStorm[2][SQUARE_MAX] = {
		// Unopposed
		{
			0,   0,   0,   0,   0,   0,   0,   0,
			0,   0,   0,   0,   0,   0,   0,   0,
		  -37, -58, -15, -30, -30,  -7, -32,  -8,
		   28,  -4, -12, -14,   0,  -1,   6,  25,
		   21,   2,  -3,   1,   9,  -5,  -5,   8,
		   16,  34,   0,   4,  15,  -8,   8,   9,
		   11,  27,   2,   2,   3,   6,   0,   7,
			0,   0,   0,   0,   0,   0,   0,   0
		},
		// Opposed
		{
			0,   0,   0,   0,   0,   0,   0,   0,
		   57,  22, -26,  17, -54, -45,  20,  83,
		  -20,-124, -82, -11, -58, -94, -79, -28,
		  -20,  -1, -34, -22, -16, -18,  -6, -16,
		   11,  16, -16, -10,  -9,  -7,   4,  -5,
			9,  45,   6,   6,   8,   3,  27,  21,
		   28,  30,   3,   4,  -3,  11,  11,  14,
			0,   0,   0,   0,   0,   0,   0,   0
		}
	};

	// Pawns can never be on the 8th rank (1st rank used as a sentinel for when there is no pawn)
	constexpr ValueType KingShieldStength[FILE_MAX / 2][RANK_MAX] = {
		{ -3, 40, 45, 26, 20, 9, 13 },
		{ -21, 22, 17, -25, -14, -5, -32 },
		{ -5, 38, 12, -1, 16, 1, -22 },
		{ -20, -7, -14, -15, -24, -33, -88 },
	};

	// Pawns can never be on the 8th rank (1st rank used as a sentinel for when there is no pawn)
	constexpr ValueType PawnStormStrength[FILE_MAX / 2][RANK_MAX] = {
		{ 45, -145, -85, 48, 27, 23, 27 },
		{ 22, -13, 62, 23, 18, -3, 11 },
		{ -3, 25, 81, 17, -1, -10, -5 },
		{ -8, -6, 50, 2, 5, -8, -11 },
	};

	constexpr ValueType BlockedStormStrength[RANK_MAX] = {
		0, 0, 36, -5, -4, -2, -1
	};

	// =======================================================================================================================================================================================
	// SPACE
	// =======================================================================================================================================================================================
	
	constexpr ValueType GetSpaceValue(int piecesCount, int safeCount)
	{
		return safeCount * safeCount * piecesCount / 30;
	}

}
