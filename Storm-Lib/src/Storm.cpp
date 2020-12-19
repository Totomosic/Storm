#include "Storm.h"

namespace Storm
{

	void Init()
	{
		Logger::Init();
		InitRays();
		InitAttacks();
		InitZobristHash();
		InitEvaluation();
		InitSearch();
	}

}
