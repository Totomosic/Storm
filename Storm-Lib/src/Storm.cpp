#include "Storm.h"

namespace Storm
{

	void Init()
	{
#ifndef EMSCRIPTEN
		Logger::Init();
#endif
		Network::Init();
		InitRays();
		InitAttacks();
		InitZobristHash();
		InitEvaluation();
		InitSearch();
	}

}
