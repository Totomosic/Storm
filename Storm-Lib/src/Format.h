#pragma once
#include "Position.h"

namespace Storm
{

	std::vector<std::string> Split(const std::string& str, const std::string& delimiter);

	class STORM_API UCI
	{
	public:
		UCI() = delete;

		static char PieceToString(Piece type, Color color);
		static SquareIndex SquareFromString(const std::string& square);
		static std::string SquareToString(SquareIndex square);
		static std::string FormatMove(Move move);
		static Move CreateMoveFromString(const Position& position, const std::string& move);
	};

}
