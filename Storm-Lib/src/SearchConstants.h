#pragma once
#include "Types.h"
#include "MoveGeneration.h"

namespace Storm
{

	constexpr int MAX_PLY = 100;

	constexpr int AspirationWindowDepth = 4;

	constexpr ValueType RazorMargin = 200;
	constexpr int RazorDepth = 3;

	constexpr ValueType GetFutilityMargin(int depth)
	{
		return 80 * depth;
	}
	constexpr int FutilityDepth = 6;

	constexpr int GetNullMoveDepthReduction(int depth, ValueType eval, ValueType beta)
	{
		return depth / 4 + 3 + std::min((eval - beta) / 80, 3);
	}
	constexpr int NullMoveDepth = 2;

	constexpr int LmrDepth = 3;
	constexpr int LmrMoveIndex = 2;

	extern int LmrReductions[MAX_PLY][MAX_MOVES];

	template<bool IsPvNode>
	inline int GetLmrReduction(bool improving, int depth, int moveIndex)
	{
		return std::max(LmrReductions[depth][moveIndex] - IsPvNode - improving, 0);
	}

}