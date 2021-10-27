#pragma once
#include "Types.h"
#include "EvalConstants.h"
#include "SearchConstants.h"
#include "SearchData.h"

#ifdef SWIG
#define STORM_API
#define ValueType int16_t
#endif

namespace Storm
{

	constexpr ValueType VALUE_GOOD_PROMOTION = 800 + MAX_HISTORY_SCORE;
	constexpr ValueType VALUE_BAD_PROMOTION = 300 + MAX_HISTORY_SCORE;
	constexpr ValueType VALUE_GOOD_CAPTURE = 1000 + MAX_HISTORY_SCORE;
	constexpr ValueType VALUE_BAD_CAPTURE = -500;
	constexpr ValueType VALUE_QUIET = 0;

	constexpr ValueType SeeThreshold = -PawnValueMg;

	constexpr ValueType CounterMoveBonus = 100 + MAX_HISTORY_SCORE;
	constexpr ValueType KillerMoveBonuses[2] = { 250 + MAX_HISTORY_SCORE, 200 + MAX_HISTORY_SCORE };

}
