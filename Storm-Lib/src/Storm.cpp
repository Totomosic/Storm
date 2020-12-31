#include "Storm.h"

namespace Storm
{

	void Init()
	{
#ifndef EMSCRIPTEN
		Logger::Init();
#endif
		InitRays();
		InitAttacks();
		InitZobristHash();
		InitEvaluation();
		InitSearch();
	}

}
