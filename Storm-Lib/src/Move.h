#pragma once
#include "Types.h"

namespace Storm
{

	STORM_API enum MoveType : uint8_t
	{
		NORMAL = 0,
		CASTLE = 1 << 0,
	};

	STORM_API enum Move : uint16_t
	{
		MOVE_NONE = 0,
	};

	constexpr Move CreateMove(SquareIndex from, SquareIndex to)
	{
		return Move((from & 0x3F) | ((to & 0x3F) << 6));
	}

	constexpr Move CreateMove(SquareIndex from, SquareIndex to, Piece promotion)
	{
		return Move((from & 0x3F) | ((to & 0x3F) << 6) | (((promotion - PIECE_KNIGHT) & 0x3) << 12));
	}

	constexpr Move CreateMove(SquareIndex from, SquareIndex to, MoveType type)
	{
		return Move((from & 0x3F) | ((to & 0x3F) << 6) | ((type & 0x1) << 15));
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
		return Piece((move >> 12) & 0x3 + PIECE_KNIGHT);
	}
	
	constexpr MoveType GetMoveType(Move move)
	{
		return MoveType((move >> 15) & 0x1);
	}

}
