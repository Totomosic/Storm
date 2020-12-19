#pragma once
#include "Types.h"
#include "EvalConstants.h"

namespace Storm
{

	constexpr ValueType VALUE_GOOD_PROMOTION = 800;
	constexpr ValueType VALUE_BAD_PROMOTION = 300;
	constexpr ValueType VALUE_GOOD_CAPTURE = 1000;
	constexpr ValueType VALUE_BAD_CAPTURE = -500;
	constexpr ValueType VALUE_QUIET = 0;

	constexpr ValueType SeeThreshold = -PawnValueEg;

	constexpr ValueType CounterMoveBonus = 100;
	constexpr ValueType KillerMoveBonuses[2] = { 250, 200 };

}
