#include "Evaluation.h"
#include "Attacks.h"

namespace Storm
{

	ValueType PieceSquareTables[COLOR_MAX][PIECE_COUNT][GAME_STAGE_MAX][SQUARE_MAX];

	void MirrorTable(ValueType* dest, const ValueType* src)
	{
		for (File file = FILE_A; file < FILE_MAX; file++)
		{
			for (Rank rank = RANK_1; rank < RANK_MAX; rank++)
			{
				SquareIndex srcIndex = CreateSquare(file, rank);
				SquareIndex dstIndex = CreateSquare(file, Rank(RANK_MAX - rank - 1));
				dest[dstIndex] = src[srcIndex];
			}
		}
	}

	template<Piece P>
	void InitPieceSquareTable(const ValueType* mgTable, const ValueType* egTable)
	{
		MirrorTable(PieceSquareTables[COLOR_WHITE][P - PIECE_START][MIDGAME], mgTable);
		MirrorTable(PieceSquareTables[COLOR_BLACK][P - PIECE_START][MIDGAME], PieceSquareTables[COLOR_WHITE][P - PIECE_START][MIDGAME]);
		MirrorTable(PieceSquareTables[COLOR_WHITE][P - PIECE_START][ENDGAME], egTable);
		MirrorTable(PieceSquareTables[COLOR_BLACK][P - PIECE_START][ENDGAME], PieceSquareTables[COLOR_WHITE][P - PIECE_START][ENDGAME]);
	}

	void InitPieceSquareTables()
	{
		InitPieceSquareTable<PIECE_PAWN>(WhitePawnsTableReversed, WhitePawnsTableReversed);
		InitPieceSquareTable<PIECE_KNIGHT>(WhiteKnightsTableReversed, WhiteKnightsTableReversed);
		InitPieceSquareTable<PIECE_BISHOP>(WhiteBishopsTableReversed, WhiteBishopsTableReversed);
		InitPieceSquareTable<PIECE_ROOK>(WhiteRooksTableReversed, WhiteRooksTableReversed);
		InitPieceSquareTable<PIECE_QUEEN>(WhiteQueensTableReversed, WhiteQueensTableReversed);
		InitPieceSquareTable<PIECE_KING>(WhiteKingsTableMidgameReversed, WhiteKingsTableEndgameReversed);
	}

	void InitEvaluation()
	{
		InitPieceSquareTables();
	}

	template<Color C>
	BitBoard GetKingAttackZone(const Position& position)
	{
		SquareIndex kingSquare = position.GetKingSquare(C);
		File file = std::clamp(FileOf(kingSquare), FILE_B, FILE_G);
		Rank rank = std::clamp(RankOf(kingSquare), RANK_2, RANK_7);

		return GetAttacks<PIECE_KING>(CreateSquare(file, rank));
	}

	template<Color C>
	void EvaluateMaterial(const Position& position, EvaluationResult& result)
	{
		int pawnCount = Popcount(position.GetPieces(C, PIECE_PAWN));
		ValueType mg = position.GetNonPawnMaterial(C) + pawnCount * PawnValueMg;
		ValueType eg = position.GetNonPawnMaterial(C) + pawnCount * PawnValueEg;
		result.Material[C][MIDGAME] = mg;
		result.Material[C][ENDGAME] = eg;
	}

	template<Color C>
	void EvaluatePawns(const Position& position, EvaluationResult& result, const EvaluationData& data)
	{
		ValueType mg = 0;
		ValueType eg = 0;

		BitBoard pawns = position.GetPieces(C, PIECE_PAWN);
		while (pawns)
		{
			SquareIndex square = PopLeastSignificantBit(pawns);
			mg += PieceSquareTables[C][PIECE_PAWN - PIECE_START][MIDGAME][square];
			eg += PieceSquareTables[C][PIECE_PAWN - PIECE_START][ENDGAME][square];
		}

		result.Pawns[C][MIDGAME] = mg;
		result.Pawns[C][ENDGAME] = eg;
	}

	template<Color C, Piece P>
	void EvaluatePieces(const Position& position, EvaluationResult& result, EvaluationData& data)
	{
		ValueType mg = 0;
		ValueType eg = 0;

		BitBoard pieces = position.GetPieces(C, P);
		while (pieces)
		{
			SquareIndex square = PopLeastSignificantBit(pieces);
			mg += PieceSquareTables[C][P - PIECE_START][MIDGAME][square];
			eg += PieceSquareTables[C][P - PIECE_START][ENDGAME][square];

			BitBoard attacks =
				P == PIECE_KNIGHT ? GetAttacks<PIECE_KNIGHT>(square) :
				P == PIECE_BISHOP ? GetAttacks<PIECE_BISHOP>(square, position.GetPieces()) :
				P == PIECE_ROOK ? GetAttacks<PIECE_ROOK>(square, position.GetPieces()) :
				P == PIECE_QUEEN ? GetAttacks<PIECE_QUEEN>(square, position.GetPieces()) : ZERO_BB;
			BitBoard kingAttacks = attacks & data.KingAttackZone[OtherColor(C)];
			int attackCount = Popcount(kingAttacks);
			data.AttackerCount[C] += bool(attackCount);
			data.AttackUnits[C] += attackCount * AttackWeights[P - PIECE_START];
		}

		result.SetPieceEval<C, P>(mg, eg);
	}

	template<Color C>
	void EvaluateKingSafety(const Position& position, EvaluationResult& result, const EvaluationData& data)
	{
		ValueType mg = 0;
		ValueType eg = 0;

		if (data.AttackerCount[OtherColor(C)] >= 2 && position.GetPieces(OtherColor(C), PIECE_QUEEN))
		{
			ValueType danger = KingSafetyTable[std::min(data.AttackUnits[OtherColor(C)], MAX_ATTACK_UNITS - 1)];
			mg -= danger;
			eg -= danger / 3;
		}

		mg += PieceSquareTables[C][PIECE_KING - PIECE_START][MIDGAME][position.GetKingSquare(C)];
		eg += PieceSquareTables[C][PIECE_KING - PIECE_START][ENDGAME][position.GetKingSquare(C)];

		result.KingSafety[C][MIDGAME] = mg;
		result.KingSafety[C][ENDGAME] = eg;
	}

	EvaluationResult EvaluateDetailed(const Position& position)
	{
		EvaluationResult result;
		result.Stage = 0;

		EvaluationData data;
		data.KingAttackZone[COLOR_WHITE] = GetKingAttackZone<COLOR_WHITE>(position);
		data.KingAttackZone[COLOR_BLACK] = GetKingAttackZone<COLOR_BLACK>(position);
		data.AttackerCount[COLOR_WHITE] = 0;
		data.AttackerCount[COLOR_BLACK] = 0;
		data.AttackUnits[COLOR_WHITE] = 0;
		data.AttackUnits[COLOR_BLACK] = 0;

		EvaluateMaterial<COLOR_WHITE>(position, result);
		EvaluateMaterial<COLOR_BLACK>(position, result);

		EvaluatePawns<COLOR_WHITE>(position, result, data);
		EvaluatePawns<COLOR_BLACK>(position, result, data);

		EvaluatePieces<COLOR_WHITE, PIECE_KNIGHT>(position, result, data);
		EvaluatePieces<COLOR_BLACK, PIECE_KNIGHT>(position, result, data);
		EvaluatePieces<COLOR_WHITE, PIECE_BISHOP>(position, result, data);
		EvaluatePieces<COLOR_BLACK, PIECE_BISHOP>(position, result, data);
		EvaluatePieces<COLOR_WHITE, PIECE_ROOK>(position, result, data);
		EvaluatePieces<COLOR_BLACK, PIECE_ROOK>(position, result, data);
		EvaluatePieces<COLOR_WHITE, PIECE_QUEEN>(position, result, data);
		EvaluatePieces<COLOR_BLACK, PIECE_QUEEN>(position, result, data);

		EvaluateKingSafety<COLOR_WHITE>(position, result, data);
		EvaluateKingSafety<COLOR_BLACK>(position, result, data);

		return result;
	}

	ValueType Evaluate(const Position& position)
	{
		return EvaluateDetailed(position).Result(position.ColorToMove);
	}

	// =================================================================================================================================
	//  FORMATTING
	// =================================================================================================================================

	std::string FormatScore(ValueType score, int maxPlaces)
	{
		int places = 1;
		if (std::abs(score) > 0)
			places = (int)log10(std::abs(score)) + 1;
		if (score < 0)
			places++;
		int spaces = maxPlaces - places;
		int left = spaces / 2;
		int right = spaces / 2;
		if (spaces % 2 == 1)
			left++;

		std::string result = "";
		for (int i = 0; i < left; i++)
			result += ' ';
		result += std::to_string(score);
		for (int i = 0; i < right; i++)
			result += ' ';
		return result;
	}

	std::string FormatEvaluation(const EvaluationResult& result)
	{
		constexpr int SCORE_LENGTH = 6;

		ValueType totals[COLOR_MAX][GAME_STAGE_MAX];
		totals[COLOR_WHITE][MIDGAME] = result.GetTotal<COLOR_WHITE, MIDGAME>();
		totals[COLOR_WHITE][ENDGAME] = result.GetTotal<COLOR_WHITE, ENDGAME>();
		totals[COLOR_BLACK][MIDGAME] = result.GetTotal<COLOR_BLACK, MIDGAME>();
		totals[COLOR_BLACK][ENDGAME] = result.GetTotal<COLOR_BLACK, ENDGAME>();

#define FORMAT_TABLE_ROW(Score, Length) (FormatScore(Score[COLOR_WHITE][MIDGAME], Length) + " " + FormatScore(Score[COLOR_WHITE][ENDGAME], Length) + " | " + FormatScore(Score[COLOR_BLACK][MIDGAME], Length) + " " + FormatScore(Score[COLOR_BLACK][ENDGAME], Length) + " | " + FormatScore(Score[COLOR_WHITE][MIDGAME] - Score[COLOR_BLACK][MIDGAME], Length) + " " + FormatScore(Score[COLOR_WHITE][ENDGAME] - Score[COLOR_BLACK][ENDGAME], Length))
		std::string format = "";
		format += "       Term     |     White     |     Black     |     Total     \n";
		format += "                |   MG     EG   |   MG     EG   |   MG     EG   \n";
		format += " ---------------+---------------+---------------+---------------\n";
		format += "       Material | " + FORMAT_TABLE_ROW(result.Material, SCORE_LENGTH) + '\n';
		format += "          Pawns | " + FORMAT_TABLE_ROW(result.Pawns, SCORE_LENGTH) + '\n';
		format += "        Knights | " + FORMAT_TABLE_ROW(result.Knights, SCORE_LENGTH) + '\n';
		format += "        Bishops | " + FORMAT_TABLE_ROW(result.Bishops, SCORE_LENGTH) + '\n';
		format += "          Rooks | " + FORMAT_TABLE_ROW(result.Rooks, SCORE_LENGTH) + '\n';
		format += "         Queens | " + FORMAT_TABLE_ROW(result.Queens, SCORE_LENGTH) + '\n';
		format += "    King Safety | " + FORMAT_TABLE_ROW(result.KingSafety, SCORE_LENGTH) + '\n';
		format += " ---------------+---------------+---------------+--------------\n";
		format += "          Total | " + FORMAT_TABLE_ROW(totals, SCORE_LENGTH) + "\n";
		format += "\n";
		format += "Game stage: " + std::to_string(result.Stage) + " / " + std::to_string(GameStageMax) + '\n';
		format += "Total evaluation: " + std::to_string(result.Result(COLOR_WHITE)) + " (white side)";
		return format;
#undef FORMAT_TABLE_ROW
	}
}
