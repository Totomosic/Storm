#pragma once
#include "EvalConstants.h"
#include "MoveGeneration.h"
#include "MoveSelectionConstants.h"

namespace Storm
{

	STORM_API enum MoveSelectionStage
	{
		FIND_TT_MOVE,
		GENERATE_CAPTURES,
		FIND_GOOD_CAPTURES,
		GENERATE_QUIETS,
		FIND_QUIETS,
		FIND_BAD_CAPTURES,
		DONE,
	};

	STORM_API enum MoveSelectorType
	{
		ALL_MOVES,
		QUIESCENCE,
	};

	struct STORM_API ValueMove
	{
	public:
		Storm::Move Move = MOVE_NONE;
		ValueType Value = VALUE_NONE;

	public:
		ValueMove() = default;
		inline ValueMove(Storm::Move mv)
			: Move(mv), Value(VALUE_NONE)
		{}
	};

	inline bool operator==(const ValueMove& left, const ValueMove& right) { return left.Move == right.Move; }
	inline bool operator!=(const ValueMove& left, const ValueMove& right) { return left.Move != right.Move; }
	inline bool operator==(const ValueMove& left, Move right) { return left.Move == right; }
	inline bool operator!=(const ValueMove& left, Move right) { return left.Move != right; }
	inline bool operator<(const ValueMove& left, const ValueMove& right) { return left.Value < right.Value; }

	template<MoveSelectorType TYPE>
	class STORM_API MoveSelector
	{
	private:
		ValueMove m_MoveBuffer[MAX_MOVES];
		ValueMove* m_Start;
		ValueMove* m_End;
		ValueMove* m_BadCapturesStart;
		ValueMove* m_BadCapturesEnd;

		MoveSelectionStage m_Stage;
		const Position& m_Position;
		Move m_HashMove;

	public:
		MoveSelector(const Position& position, Move hashMove);

		Move GetNextMove();

	private:
		void NextStage();
		void GenerateCaptures();
		void GenerateQuiets();
	};

}
