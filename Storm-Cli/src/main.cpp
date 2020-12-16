#include "Storm.h"
#include "CommandManager.h"

using namespace Storm;

int main()
{
	Init();
	std::string version = __TIMESTAMP__;

	std::cout << "Storm " << version << " by J. Morrison" << std::endl;

	CommandManager commands;

	Position position = CreatePositionFromFEN("rnbqkbnr/ppp2ppp/3p4/1B2p3/3PP3/8/PPP2PPP/RNBQK1NR b KQkq - 1 3");
	std::cout << position << std::endl;

	Move moves[MAX_MOVES];
	MoveList list(moves);

	list.Fill(GenerateAll<COLOR_BLACK, EVASIONS>(position, list.GetStart()));

	for (Move move : list)
		std::cout << UCI::FormatMove(move) << std::endl;
	std::cout << list.Size() << std::endl;

	char buffer[8192];
	while (true)
	{
		std::cin.getline(buffer, sizeof(buffer));
		commands.ExecuteCommand(buffer);
	}
	return 0;
}
