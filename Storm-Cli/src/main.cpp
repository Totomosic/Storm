#include "Storm.h"
#include "CommandManager.h"

using namespace Storm;

int main()
{
	Init();
	std::string version = __TIMESTAMP__;

	std::cout << "Storm " << version << " by J. Morrison" << std::endl;

	CommandManager commands;

	char buffer[8192];
	while (true)
	{
		std::cin.getline(buffer, sizeof(buffer));
		commands.ExecuteCommand(buffer);
	}
	return 0;
}
