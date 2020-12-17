#include "MoveSelection.h"
#include "Position.h"

namespace Storm
{

	template<MoveSelectorType TYPE>
	MoveSelector<TYPE>::MoveSelector(const Position& position, Move hashMove)
		: m_MoveBuffer(), m_Start(nullptr), m_End(nullptr), m_BadCapturesStart(nullptr), m_Stage(FIND_TT_MOVE), m_Position(position), m_HashMove(hashMove)
	{
		if (m_HashMove == MOVE_NONE)
			NextStage();
	}

	template<MoveSelectorType TYPE>
	Move MoveSelector<TYPE>::GetNextMove()
	{
		switch (m_Stage)
		{
		case FIND_TT_MOVE:
			if (m_Position.IsPseudoLegal(m_HashMove))
			{
				NextStage();
				return m_HashMove;
			}
			NextStage();
			return GetNextMove();
		case GENERATE_CAPTURES:
			GenerateCaptures();
			NextStage();
			return GetNextMove();
		case FIND_GOOD_CAPTURES:
			while (m_Start != m_End)
			{
				if (*m_Start != m_HashMove)
					return (m_Start++)->Move;
				m_Start++;
			}
			NextStage();
			return GetNextMove();
		case GENERATE_QUIETS:
			GenerateQuiets();
			NextStage();
			return GetNextMove();
		case FIND_QUIETS:
			while (m_Start != m_End)
			{
				if (*m_Start != m_HashMove)
					return (m_Start++)->Move;
				m_Start++;
			}
			NextStage();
			return GetNextMove();
		case FIND_BAD_CAPTURES:
			while (m_BadCapturesStart != m_BadCapturesEnd)
			{
				if (*m_BadCapturesStart != m_HashMove)
					return (m_BadCapturesStart++)->Move;
				m_BadCapturesStart++;
			}
			NextStage();
			return GetNextMove();
		default:
			break;
		}
		return MOVE_NONE;
	}

	template<MoveSelectorType TYPE>
	void MoveSelector<TYPE>::NextStage()
	{
		if (m_Stage == FIND_QUIETS && TYPE == QUIESCENCE)
		{
			m_Stage = DONE;
		}
		else if (m_Stage == FIND_GOOD_CAPTURES && !m_Position.InCheck() && TYPE == QUIESCENCE)
		{
			m_Stage = DONE;
		}
		else
		{
			// Default increment stage
			m_Stage = MoveSelectionStage(m_Stage + 1);
		}
	}

	template<MoveSelectorType TYPE>
	void MoveSelector<TYPE>::GenerateCaptures()
	{
		ValueMove* end;
		m_Start = m_MoveBuffer;
		m_End = m_Start;
		if (m_Position.ColorToMove == COLOR_WHITE)
			end = GenerateAll<COLOR_WHITE, CAPTURES>(m_Position, m_Start);
		else
			end = GenerateAll<COLOR_BLACK, CAPTURES>(m_Position, m_Start);
		m_BadCapturesEnd = std::end(m_MoveBuffer);
		m_BadCapturesStart = m_BadCapturesEnd;
		ValueMove* it = m_Start;
		while (it != end)
		{
			if (m_Position.SeeGE(it->Move))
			{
				it->Value = VALUE_GOOD_CAPTURE;
				std::swap(*it, *m_End++);
			}
			else
			{
				it->Value = VALUE_BAD_CAPTURE;
				std::swap(*it, *(--m_BadCapturesStart));
			}
			++it;
		}
	}

	template<MoveSelectorType TYPE>
	void MoveSelector<TYPE>::GenerateQuiets()
	{
		m_Start = m_MoveBuffer;
		if (m_Position.ColorToMove == COLOR_WHITE)
		{
			if (m_Position.InCheck())
				m_End = GenerateAll<COLOR_WHITE, EVASIONS>(m_Position, m_Start);
			else
			{
				STORM_ASSERT(TYPE != QUIESCENCE, "Invalid");
				m_End = GenerateAll<COLOR_WHITE, QUIETS>(m_Position, m_Start);
			}
		}
		else
		{
			if (m_Position.InCheck())
				m_End = GenerateAll<COLOR_BLACK, EVASIONS>(m_Position, m_Start);
			else
			{
				STORM_ASSERT(TYPE != QUIESCENCE, "Invalid");
				m_End = GenerateAll<COLOR_BLACK, QUIETS>(m_Position, m_Start);
			}
		}

		ValueMove* it = m_Start;
		while (it != m_Start)
		{
			it->Value = VALUE_QUIET;
			++it;
		}
	}

	template class MoveSelector<ALL_MOVES>;
	template class MoveSelector<QUIESCENCE>;

}
