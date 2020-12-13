#pragma once
#include "Position.h"

namespace Storm
{

	class STORM_API UCI
	{
	public:
		UCI() = delete;

		static char PieceToString(Piece type, Color color);
		static SquareIndex SquareFromString(const std::string& square);
		static std::string SquareToString(SquareIndex square);
		static std::string FormatMove(Move move);
	};

}
