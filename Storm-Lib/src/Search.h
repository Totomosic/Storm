#pragma once
#include "Position.h"
#include "MoveGeneration.h"
#include "Format.h"

namespace Storm
{

	class STORM_API Search
	{
	private:
	public:
		Search();

		size_t Perft(const Position& position, int depth);

	private:
		size_t PerftPosition(Position& position, int depth);

	};

}
