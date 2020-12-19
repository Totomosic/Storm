#pragma once
#include "Types.h"

namespace Storm
{

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

	constexpr ValueType Initiative = 10;

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
	constexpr int KnightAttackWeight = 6;
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

}
