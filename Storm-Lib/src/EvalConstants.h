#pragma once
#include "Types.h"

namespace Storm
{

	constexpr ValueType VALUE_MATE = 100000;
	constexpr ValueType VALUE_NONE = -VALUE_MATE - 100;
	constexpr ValueType VALUE_DRAW = 0;
	
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

	constexpr ValueType PIECE_VALUES_MG[PIECE_COUNT] = {
		PawnValueMg,
		KnightValueMg,
		BishopValueMg,
		RookValueMg,
		QueenValueMg,
		KingValueMg,
	};

	constexpr ValueType PIECE_VALUES_EG[PIECE_COUNT] = {
		PawnValueEg,
		KnightValueEg,
		BishopValueEg,
		RookValueEg,
		QueenValueEg,
		KingValueEg,
	};

	constexpr ValueType GetPieceValueMg(Piece piece)
	{
		return PIECE_VALUES_MG[piece - PIECE_START];
	}

	constexpr ValueType GetPieceValueEg(Piece piece)
	{
		return PIECE_VALUES_EG[piece - PIECE_START];
	}

}
