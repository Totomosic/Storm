#pragma once
#include "EvalConstants.h"
#include "MoveGeneration.h"
#include "MoveSelectionConstants.h"

namespace Storm
{

	STORM_API enum MoveSelectionStage
	{
		TT_MOVE,
		GENERATE_CAPTURES,
		GOOD_CAPTURES,
		GENERATE_QUIETS,
		CHECK_EVASIONS,
		QUIETS,
		BAD_CAPTURES,
	};

	STORM_API enum MoveSelectorType
	{
		NORMAL,
		QUIESCENCE,
	};

	class STORM_API MoveSelector
	{
	private:

	public:
		MoveSelector();
	};

}
