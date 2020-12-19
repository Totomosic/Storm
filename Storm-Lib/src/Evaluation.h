#pragma once
#include "Position.h"
#include "EvalConstants.h"

namespace Storm
{

	struct STORM_API EvaluationData
	{
	public:
		BitBoard KingAttackZone[COLOR_MAX];
		int AttackerCount[COLOR_MAX];
		int AttackUnits[COLOR_MAX];

		// Includes PIECE_ALL
		BitBoard AttackedBy[COLOR_MAX][PIECE_MAX + 2];
		BitBoard AttackedByTwice[COLOR_MAX];
	};

	struct STORM_API EvaluationResult
	{
	public:
		ValueType Material[COLOR_MAX][GAME_STAGE_MAX];
		ValueType Pawns[COLOR_MAX][GAME_STAGE_MAX];
		ValueType Knights[COLOR_MAX][GAME_STAGE_MAX];
		ValueType Bishops[COLOR_MAX][GAME_STAGE_MAX];
		ValueType Rooks[COLOR_MAX][GAME_STAGE_MAX];
		ValueType Queens[COLOR_MAX][GAME_STAGE_MAX];
		ValueType KingSafety[COLOR_MAX][GAME_STAGE_MAX];
		ValueType Space[COLOR_MAX][GAME_STAGE_MAX];
		int Stage;

	public:
		template<Color C, GameStage S>
		inline ValueType GetTotal() const
		{
			return
				Material[C][S] +
				Pawns[C][S] +
				Knights[C][S] +
				Bishops[C][S] +
				Rooks[C][S] +
				Queens[C][S] +
				KingSafety[C][S] +
				Space[C][S];
		}

		template<GameStage S>
		inline ValueType GetStageTotal() const
		{
			return GetTotal<COLOR_WHITE, S>() - GetTotal<COLOR_BLACK, S>();
		}

		inline ValueType Result(Color sideToMove) const
		{
			ValueType mg = GetStageTotal<MIDGAME>();
			ValueType eg = GetStageTotal<ENDGAME>();
			ValueType result = (mg * (GameStageMax - Stage) + eg * Stage) / GameStageMax;
			return (sideToMove == COLOR_WHITE ? result : -result) + Initiative;
		}

		template<Color C, Piece PIECE>
		void SetPieceEval(ValueType mg, ValueType eg)
		{
			if constexpr (PIECE == PIECE_KNIGHT)
			{
				Knights[C][MIDGAME] = mg;
				Knights[C][ENDGAME] = eg;
			}
			if constexpr (PIECE == PIECE_BISHOP)
			{
				Bishops[C][MIDGAME] = mg;
				Bishops[C][ENDGAME] = eg;
			}
			if constexpr (PIECE == PIECE_ROOK)
			{
				Rooks[C][MIDGAME] = mg;
				Rooks[C][ENDGAME] = eg;
			}
			if constexpr (PIECE == PIECE_QUEEN)
			{
				Queens[C][MIDGAME] = mg;
				Queens[C][ENDGAME] = eg;
			}
		}
	};

	void InitEvaluation();

	EvaluationResult EvaluateDetailed(const Position& position);
	ValueType Evaluate(const Position& position);

	template<Color C>
	void EvaluateMaterial(const Position& position, EvaluationResult& result);

	template<Color C>
	void EvaluatePawns(const Position& position, EvaluationResult& result, const EvaluationData& data);

	template<Color C, Piece P>
	void EvaluatePieces(const Position& position, EvaluationResult& result, EvaluationData& data);

	template<Color C>
	void EvaluateKingSafety(const Position& position, EvaluationResult& result, const EvaluationData& data);

	template<Color C>
	void EvaluateSpace(const Position& position, EvaluationResult& result, const EvaluationData& data);

	std::string FormatEvaluation(const EvaluationResult& result);

}
