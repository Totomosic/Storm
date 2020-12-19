#include "MoveSelection.h"
#include "Position.h"

namespace Storm
{

	template<MoveSelectorType TYPE>
	MoveSelector<TYPE>::MoveSelector(const Position& position, Move hashMove, Move counterMove, Move killers[2])
		: m_Stage(FIND_TT_MOVE), m_Position(position), m_HashMove(hashMove), m_CounterMove(counterMove), m_Killers(killers)
	{
		STORM_ASSERT(TYPE == ALL_MOVES, "Invalid Type");
		if (m_HashMove == MOVE_NONE)
			NextStage();
	}

	template<MoveSelectorType TYPE>
	MoveSelector<TYPE>::MoveSelector(const Position& position)
		: m_Stage(FIND_TT_MOVE), m_Position(position), m_HashMove(MOVE_NONE)
	{
		STORM_ASSERT(TYPE == QUIESCENCE, "Invalid Type");
		if (m_HashMove == MOVE_NONE)
			NextStage();
	}

	template<MoveSelectorType TYPE>
	Move MoveSelector<TYPE>::GetNextMove()
	{
top:
		switch (m_Stage)
		{
		case FIND_TT_MOVE:
			if (m_Position.IsPseudoLegal(m_HashMove))
			{
				NextStage();
				return m_HashMove;
			}
			NextStage();
			goto top;
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
			goto top;
		case GENERATE_QUIETS:
			GenerateQuiets();
			NextStage();
			goto top;
		case FIND_QUIETS:
			while (m_Start != m_End)
			{
				if (*m_Start != m_HashMove)
					return (m_Start++)->Move;
				m_Start++;
			}
			NextStage();
			goto top;
		case FIND_BAD_CAPTURES:
			while (m_BadCapturesStart != m_BadCapturesEnd)
			{
				if (*m_BadCapturesStart != m_HashMove)
					return (m_BadCapturesStart++)->Move;
				m_BadCapturesStart++;
			}
			NextStage();
			goto top;
		default:
			break;
		}
		return MOVE_NONE;
	}

	template<MoveSelectorType TYPE>
	void MoveSelector<TYPE>::NextStage()
	{
		if (m_Stage == FIND_GOOD_CAPTURES && !m_Position.InCheck() && TYPE == QUIESCENCE)
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
		{
			if (m_Position.InCheck())
				end = GenerateAll<COLOR_WHITE, EVASION_CAPTURES>(m_Position, m_Start);
			else
				end = GenerateAll<COLOR_WHITE, CAPTURES>(m_Position, m_Start);
		}
		else
		{
			if (m_Position.InCheck())
				end = GenerateAll<COLOR_BLACK, EVASION_CAPTURES>(m_Position, m_Start);
			else
				end = GenerateAll<COLOR_BLACK, CAPTURES>(m_Position, m_Start);
		}
		m_BadCapturesEnd = std::end(m_MoveBuffer);
		m_BadCapturesStart = m_BadCapturesEnd;
		ValueMove* it = m_Start;
		while (it != end)
		{
			if (m_Position.SeeGE(it->Move))
			{
				it->Value = VALUE_GOOD_CAPTURE + GetPieceValueMg(m_Position.GetCapturedPiece(it->Move)) - GetPieceValueMg(m_Position.GetMovingPiece(it->Move));
				if (GetMoveType(it->Move) == PROMOTION)
					it->Value += GetPieceValueMg(GetPromotionPiece(it->Move));
				std::swap(*it, *m_End++);
			}
			else
			{
				it->Value = VALUE_BAD_CAPTURE + GetPieceValueMg(m_Position.GetCapturedPiece(it->Move)) - GetPieceValueMg(m_Position.GetMovingPiece(it->Move));
				if (GetMoveType(it->Move) == PROMOTION)
					it->Value += GetPieceValueMg(GetPromotionPiece(it->Move));
				std::swap(*it, *(--m_BadCapturesStart));
			}
			++it;
		}
		std::sort(m_Start, m_End, std::greater<ValueMove>());
		std::sort(m_BadCapturesStart, m_BadCapturesEnd, std::greater<ValueMove>());
	}

	template<MoveSelectorType TYPE>
	void MoveSelector<TYPE>::GenerateQuiets()
	{
		m_Start = m_MoveBuffer;
		if (m_Position.ColorToMove == COLOR_WHITE)
		{
			if (m_Position.InCheck())
				m_End = GenerateAll<COLOR_WHITE, EVASION_QUIETS>(m_Position, m_Start);
			else
			{
				STORM_ASSERT(TYPE != QUIESCENCE, "Invalid");
				m_End = GenerateAll<COLOR_WHITE, QUIETS>(m_Position, m_Start);
			}
		}
		else
		{
			if (m_Position.InCheck())
				m_End = GenerateAll<COLOR_BLACK, EVASION_QUIETS>(m_Position, m_Start);
			else
			{
				STORM_ASSERT(TYPE != QUIESCENCE, "Invalid");
				m_End = GenerateAll<COLOR_BLACK, QUIETS>(m_Position, m_Start);
			}
		}

		ValueMove* it = m_Start;
		while (it != m_End)
		{
			it->Value = VALUE_QUIET;
			if (GetMoveType(it->Move) == PROMOTION)
			{
				Piece promotion = GetPromotionPiece(it->Move);
				if (promotion == PIECE_QUEEN || promotion == PIECE_KNIGHT)
					it->Value += VALUE_GOOD_PROMOTION + GetPieceValueMg(promotion);
				else
					it->Value += VALUE_BAD_PROMOTION + GetPieceValueMg(promotion);
			}
			if constexpr (TYPE == ALL_MOVES)
			{
				if (*it == m_CounterMove)
					it->Value += CounterMoveBonus;
				if (*it == m_Killers[0])
					it->Value += KillerMoveBonuses[0];
				else if (*it == m_Killers[1])
					it->Value += KillerMoveBonuses[1];
			}
			++it;
		}

		std::sort(m_Start, m_End, std::greater<ValueMove>());
	}

	template class MoveSelector<ALL_MOVES>;
	template class MoveSelector<QUIESCENCE>;

}
