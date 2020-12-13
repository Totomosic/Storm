#include "Storm.h"

using namespace Storm;

int main()
{
	Init();

	Position position = CreateStartingPosition();//CreatePositionFromFEN("8/3kb3/6Q1/p3B3/6p1/2PP1q2/P4PKP/8 w - - 17 37");

	std::cout << position << std::endl;

	Move moves[255];
	Move* end = GenerateAll<COLOR_WHITE, ALL>(position, moves);

	Move* start = moves;
	while (start != end)
	{
		std::cout << UCI::FormatMove(*start++) << std::endl;
	}

	position.ApplyMove(moves[19]);
	std::cout << position << std::endl;

	return 0;
}
