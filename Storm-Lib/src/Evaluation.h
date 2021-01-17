#pragma once
#include "Position.h"
#include "EvalConstants.h"

namespace Storm
{

	// =======================================================================================================================================================================================
	// UTILS
	// =======================================================================================================================================================================================

	constexpr int SignOf(ValueType value)
	{
		return int((value > 0) - (value < 0));
	}

	template<Color C>
	inline SquareIndex FrontmostSquare(BitBoard board)
	{
		return C == COLOR_WHITE ? MostSignificantBit(board) : LeastSignificantBit(board);
	}

	constexpr BitBoard InFrontRanksWhite[RANK_MAX] = {
		ALL_SQUARES_BB,
		ALL_SQUARES_BB & ~RANK_1_BB,
		ALL_SQUARES_BB & ~RANK_1_BB & ~RANK_2_BB,
		ALL_SQUARES_BB & ~RANK_1_BB & ~RANK_2_BB & ~RANK_3_BB,
		ALL_SQUARES_BB & ~RANK_1_BB & ~RANK_2_BB & ~RANK_3_BB & ~RANK_4_BB,
		ALL_SQUARES_BB & ~RANK_1_BB & ~RANK_2_BB & ~RANK_3_BB & ~RANK_4_BB & ~RANK_5_BB,
		ALL_SQUARES_BB & ~RANK_1_BB & ~RANK_2_BB & ~RANK_3_BB & ~RANK_4_BB & ~RANK_5_BB & ~RANK_6_BB,
		ALL_SQUARES_BB & ~RANK_1_BB & ~RANK_2_BB & ~RANK_3_BB & ~RANK_4_BB & ~RANK_5_BB & ~RANK_6_BB & ~RANK_7_BB,
	};

	constexpr BitBoard InFrontRanksBlack[RANK_MAX] = {
		ALL_SQUARES_BB & ~RANK_8_BB & ~RANK_7_BB & ~RANK_6_BB & ~RANK_5_BB & ~RANK_4_BB & ~RANK_3_BB & ~RANK_2_BB,
		ALL_SQUARES_BB & ~RANK_8_BB & ~RANK_7_BB & ~RANK_6_BB & ~RANK_5_BB & ~RANK_4_BB & ~RANK_3_BB,
		ALL_SQUARES_BB & ~RANK_8_BB & ~RANK_7_BB & ~RANK_6_BB & ~RANK_5_BB & ~RANK_4_BB,
		ALL_SQUARES_BB & ~RANK_8_BB & ~RANK_7_BB & ~RANK_6_BB & ~RANK_5_BB,
		ALL_SQUARES_BB & ~RANK_8_BB & ~RANK_7_BB & ~RANK_6_BB,
		ALL_SQUARES_BB & ~RANK_8_BB & ~RANK_7_BB,
		ALL_SQUARES_BB & ~RANK_8_BB,
		ALL_SQUARES_BB,
	};

	template<Color C>
	constexpr BitBoard InFrontOrEqual(Rank rank)
	{
		return C == COLOR_WHITE ? InFrontRanksWhite[rank] : InFrontRanksBlack[rank];
	}

	// =======================================================================================================================================================================================
	// EVAL
	// =======================================================================================================================================================================================

	struct STORM_API EvaluationData
	{
	public:
		BitBoard KingAttackZone[COLOR_MAX];
		int AttackerCount[COLOR_MAX];
		int AttackUnits[COLOR_MAX];
		BitBoard CheckSquares[COLOR_MAX];

		// Includes PIECE_ALL
		BitBoard AttackedBy[COLOR_MAX][PIECE_MAX + 2];
		BitBoard AttackedByTwice[COLOR_MAX];

		BitBoard MobilityArea[COLOR_MAX];
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
		ValueType Initiative;
		int Stage;

	public:
		template<Color C, GameStage S>
		inline ValueType GetTotal() const
		{
			ValueType eval =
				Material[C][S] +
				Pawns[C][S] +
				Knights[C][S] +
				Bishops[C][S] +
				Rooks[C][S] +
				Queens[C][S] +
				KingSafety[C][S] +
				Space[C][S];
			if constexpr (S == ENDGAME)
			{
				eval += SignOf(eval) * std::max(Initiative, -std::abs(eval));
			}
			return eval;
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

			constexpr SquareIndex index = CreateSquare(FILE_B, RANK_7);
			constexpr ValueType value = PawnShield[index];

			return (sideToMove == COLOR_WHITE ? result : -result) + Tempo;
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

	inline bool InsufficientMaterial(const Position& position)
	{
		BitBoard pieces = position.GetPieces();
		return Popcount(pieces) <= 3 && (pieces & position.GetPieces(PIECE_KNIGHT, PIECE_BISHOP));
	}

}
