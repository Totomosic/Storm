#include "Storm.h"

namespace Storm
{

	void Init(const std::string& evalFilename)
	{
#ifndef EMSCRIPTEN
		Logger::Init();
#endif
		InitRays();
		InitAttacks();
		InitZobristHash();
		InitEvaluation(evalFilename);
		InitSearch();
	}

}
