#pragma once
#include "Types.h"

namespace Storm
{

	STORM_API enum Move : uint16_t
	{
		MOVE_NONE = 0,
	};

	constexpr Move CreateMove(SquareIndex from, SquareIndex to, Piece promotion = PIECE_QUEEN)
	{
		return Move((from & 0x3F) | ((to & 0x3F) << 6) | ((promotion & 0x7) << 12));
	}

	constexpr SquareIndex GetFromSquare(Move move)
	{
		return SquareIndex(move & 0x3F);
	}

	constexpr SquareIndex GetToSquare(Move move)
	{
		return SquareIndex((move >> 6) & 0x3F);
	}

	constexpr Piece GetPromotionPiece(Move move)
	{
		return Piece((move >> 12) & 0x7);
	}

}
