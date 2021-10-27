#include "Evaluation.h"
#include "Attacks.h"
#include "Format.h"

#include "nnue/evaluate_nnue.h"
#include <fstream>
#include <filesystem>

namespace Storm
{

	static bool NNUEAvailable = false;
	static std::string NNUEFilename = "";

	// ValueType PieceSquareTables[COLOR_MAX][PIECE_COUNT][GAME_STAGE_MAX][SQUARE_MAX];
	BitBoard PassedPawnMasks[COLOR_MAX][SQUARE_MAX];
	BitBoard SupportedPawnMasks[COLOR_MAX][SQUARE_MAX];

	void InitPassedPawnMasks()
	{
		for (SquareIndex square = a1; square < SQUARE_MAX; square++)
		{
			BitBoard north = GetRay(NORTH, square);
			BitBoard south = GetRay(SOUTH, square);

			PassedPawnMasks[COLOR_WHITE][square] = north | Shift<EAST>(north) | Shift<WEST>(north);
			PassedPawnMasks[COLOR_BLACK][square] = south | Shift<EAST>(south) | Shift<WEST>(south);

			BitBoard sq = GetSquareBB(square);
			SupportedPawnMasks[COLOR_WHITE][square] = Shift<EAST>(sq) | Shift<WEST>(sq) | Shift<SOUTH_EAST>(sq) | Shift<SOUTH_WEST>(sq);
			SupportedPawnMasks[COLOR_BLACK][square] = Shift<EAST>(sq) | Shift<WEST>(sq) | Shift<NORTH_EAST>(sq) | Shift<NORTH_WEST>(sq);
		}
	}

	void InitEvaluation(const std::string& evalFilename)
	{
		InitPassedPawnMasks();

		std::string networkFilename = "";
		bool networkLoaded = false;
		if (!evalFilename.empty())
		{
			std::ifstream stream(evalFilename, std::ios::binary);
			networkLoaded = NNUE::load_eval(evalFilename, stream);
			if (networkLoaded)
				networkFilename = evalFilename;
		}
		if (!networkLoaded)
		{
			for (auto entry : std::filesystem::directory_iterator("."))
			{
				if (entry.is_regular_file() && entry.path().extension() == ".nnue")
				{
					std::ifstream stream(entry.path().string(), std::ios::binary);
					networkLoaded = NNUE::load_eval(entry.path().string(), stream);
					if (networkLoaded)
					{
						networkFilename = entry.path().string();
						break;
					}
				}
			}
		}
		if (networkLoaded)
			std::cout << "Loaded network " << networkFilename << std::endl;
		else
			std::cout << "Failed to load NNUE parameters" << std::endl;
		NNUEAvailable = networkLoaded;
		NNUEFilename = networkFilename;
	}

	bool IsNNUEAvailable()
	{
		return NNUEAvailable;
	}

	std::string GetNNUEFilename()
	{
		return NNUEFilename;
	}

	template<Color C>
	BitBoard GetKingAttackZone(const Position& position)
	{
		SquareIndex kingSquare = position.GetKingSquare(C);
		File file = std::clamp(FileOf(kingSquare), FILE_B, FILE_G);
		Rank rank = std::clamp(RankOf(kingSquare), RANK_2, RANK_7);

		return GetAttacks<PIECE_KING>(CreateSquare(file, rank)) | kingSquare;
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
		constexpr Direction Up = C == COLOR_WHITE ? NORTH : SOUTH;

		ValueType mg = 0;
		ValueType eg = 0;

		BitBoard pawns = position.GetPieces(C, PIECE_PAWN);
		BitBoard enemyPawns = position.GetPieces(OtherColor(C), PIECE_PAWN);
		while (pawns)
		{
			SquareIndex square = PopLeastSignificantBit(pawns);
			mg += GetPieceSquareValue<C, PIECE_PAWN, MIDGAME>(square);
			eg += GetPieceSquareValue<C, PIECE_PAWN, ENDGAME>(square);

			if (IsPassedPawn<C>(square, enemyPawns))
			{
				Rank rank = RankOf(square);
				mg += GetPassedPawnValue<C, MIDGAME>(rank);
				eg += GetPassedPawnValue<C, ENDGAME>(rank);
				if (IsSupportedPawn<C>(square, pawns))
				{
					mg += SupportedPassedPawn[MIDGAME];
					eg += SupportedPassedPawn[ENDGAME];
				}
			}
		}

		BitBoard safeArea = data.AttackedBy[C][PIECE_ALL] & ~data.AttackedBy[OtherColor(C)][PIECE_ALL];
		BitBoard safePawnAttacks = GetPawnAttacks<C>(position.GetPieces(C, PIECE_PAWN) & safeArea);
		BitBoard nonPawnEnemies = position.GetPieces(OtherColor(C)) & ~position.GetPieces(OtherColor(C), PIECE_PAWN);
		int threatenedPieces = Popcount(nonPawnEnemies & safePawnAttacks);

		mg += threatenedPieces * ThreatByProtectedPawn[MIDGAME];
		eg += threatenedPieces * ThreatByProtectedPawn[ENDGAME];

		BitBoard pushedSafePawnAttacks = GetPawnAttacks<C>(Shift<Up>(position.GetPieces(C, PIECE_PAWN)) & safeArea);
		int threatenedByPush = Popcount(nonPawnEnemies & pushedSafePawnAttacks);

		mg += threatenedByPush * ThreatByProtectedPushedPawn[MIDGAME];
		eg += threatenedByPush * ThreatByProtectedPushedPawn[ENDGAME];

		for (File file = FILE_A; file < FILE_MAX; file++)
		{
			if (MoreThanOne(FILE_MASKS[file] & position.GetPieces(C, PIECE_PAWN)))
			{
				mg -= DoubledPawnPenalty[MIDGAME];
				eg -= DoubledPawnPenalty[ENDGAME];
			}
		}

		result.Pawns[C][MIDGAME] = mg;
		result.Pawns[C][ENDGAME] = eg;
	}

	template<Color C, Piece P>
	void ScoreThreats(const Position& position, EvaluationData& data, BitBoard attacks, SquareIndex square, ValueType& mg, ValueType& eg)
	{
		BitBoard threatenedPieces = attacks & position.GetPieces(OtherColor(C)) & ~position.GetKingSquare(OtherColor(C));
		while (threatenedPieces)
		{
			SquareIndex enemySquare = PopLeastSignificantBit(threatenedPieces);
			mg += GetThreatBonus<P, MIDGAME>(TypeOf(position.GetPieceOnSquare(enemySquare)));
			eg += GetThreatBonus<P, ENDGAME>(TypeOf(position.GetPieceOnSquare(enemySquare)));
		}
	}

	template<Color C, Piece P>
	void EvaluatePieces(const Position& position, EvaluationResult& result, EvaluationData& data)
	{
		constexpr Direction Down = C == COLOR_WHITE ? SOUTH : NORTH;

		ValueType mg = 0;
		ValueType eg = 0;

		data.AttackedBy[C][P] = ZERO_BB;

		BitBoard outpostSquares;
		if constexpr (P == PIECE_BISHOP || P == PIECE_KNIGHT)
		{
			outpostSquares = data.AttackedBy[C][PIECE_PAWN];
			outpostSquares &= OutpostZone[C];
			BitBoard b = outpostSquares;
			while (b)
			{
				SquareIndex square = PopLeastSignificantBit(b);
				File file = FileOf(square);
				Rank rank = RankOf(square);
				if (data.AttackedBy[OtherColor(C)][PIECE_PAWN] & (FILE_MASKS[file] & InFrontOrEqual<C>(rank)))
				{
					outpostSquares &= ~FILE_MASKS[file];
					b &= ~FILE_MASKS[file];
				}
			}
		}

		BitBoard pieces = position.GetPieces(C, P);

		if constexpr (P == PIECE_BISHOP)
		{
			if (MoreThanOne(pieces))
			{
				mg += BishopPairBonus[MIDGAME];
				eg += BishopPairBonus[ENDGAME];
			}
		}

		BitBoard checkSquares = 
			P == PIECE_KNIGHT ? GetAttacks<PIECE_KNIGHT>(position.GetKingSquare(OtherColor(C))) :
			P == PIECE_BISHOP ? GetAttacks<PIECE_BISHOP>(position.GetKingSquare(OtherColor(C)), position.GetPieces()) :
			P == PIECE_ROOK ? GetAttacks<PIECE_ROOK>(position.GetKingSquare(OtherColor(C)), position.GetPieces()) :
			P == PIECE_QUEEN ? GetAttacks<PIECE_QUEEN>(position.GetKingSquare(OtherColor(C)), position.GetPieces()) : ZERO_BB;

		while (pieces)
		{
			result.Stage -= GameStageWeights[P - PIECE_START];
			SquareIndex square = PopLeastSignificantBit(pieces);
			mg += GetPieceSquareValue<C, P, MIDGAME>(square);
			eg += GetPieceSquareValue<C, P, ENDGAME>(square);

			BitBoard attacks =
				P == PIECE_KNIGHT ? GetAttacks<PIECE_KNIGHT>(square) :
				P == PIECE_BISHOP ? GetAttacks<PIECE_BISHOP>(square, position.GetPieces()) :
				P == PIECE_ROOK ? GetAttacks<PIECE_ROOK>(square, position.GetPieces()) :
				P == PIECE_QUEEN ? GetAttacks<PIECE_QUEEN>(square, position.GetPieces()) : ZERO_BB;

			if (position.GetBlockersForKing(C) & square)
				attacks &= GetLineBetween(position.GetKingSquare(C), square);

			data.AttackedBy[C][P] |= attacks;
			data.AttackedByTwice[C] |= data.AttackedBy[C][PIECE_ALL] & attacks;
			data.AttackedBy[C][PIECE_ALL] |= attacks;

			attacks &= data.MobilityArea[C];

			int reachableSquares = Popcount(attacks);
			mg += GetMobilityBonus<P, MIDGAME>(reachableSquares);
			eg += GetMobilityBonus<P, ENDGAME>(reachableSquares);

			if constexpr (P != PIECE_QUEEN)
			{
				ScoreThreats<C, P>(position, data, attacks, square, mg, eg);
			}

			if constexpr (P == PIECE_BISHOP || P == PIECE_KNIGHT)
			{
				if (outpostSquares & square)
				{
					// Knight/Bishop on outpost square
					mg += OutpostBonus[P == PIECE_KNIGHT ? 0 : 1];
					eg += OutpostBonus[P == PIECE_KNIGHT ? 0 : 1];
				}
				else if (outpostSquares & attacks)
				{
					// Knight/Bishop threatening to go to outpost square
					mg += OutpostBonus[P == PIECE_KNIGHT ? 0 : 1] / 2;
					eg += OutpostBonus[P == PIECE_KNIGHT ? 0 : 1] / 2;
				}

				if (Shift<Down>(position.GetPieces(C, PIECE_PAWN)) & square & InFrontOrEqual<C>(RelativeRank<C>(RANK_3)))
					mg += MinorBehindPawnBonus;

				if constexpr (P == PIECE_BISHOP)
				{
					if (MoreThanOne(GetAttacks<PIECE_BISHOP>(square, position.GetPieces(PIECE_PAWN)) & Center))
						mg += BishopTargetingCenterBonus;
				}
			}
			if constexpr (P == PIECE_ROOK)
			{
				int pawnCountOnFile = Popcount(FILE_MASKS[FileOf(square)] & position.GetPieces(PIECE_PAWN));
				if (pawnCountOnFile < 2)
				{
					mg += RookOnOpenFileBonus[pawnCountOnFile == 0 ? 0 : 1][MIDGAME];
					eg += RookOnOpenFileBonus[pawnCountOnFile == 0 ? 0 : 1][ENDGAME];
				}
			}
			if constexpr (P == PIECE_QUEEN)
			{
				BitBoard pinners;
				if (position.GetSliderBlockers(position.GetPieces(OtherColor(C), PIECE_ROOK, PIECE_BISHOP), square, &pinners))
				{
					mg += QueenXRayed[MIDGAME];
					eg += QueenXRayed[ENDGAME];
				}
			}

			BitBoard kingAttacks = attacks & (data.KingAttackZone[OtherColor(C)] | checkSquares);
			if (kingAttacks)
			{
				int attackCount = Popcount(kingAttacks & data.KingAttackZone[OtherColor(C)]);
				data.AttackerCount[C]++;
				data.AttackUnits[C] += attackCount * GetAttackWeight(P);

				BitBoard checkingMoves = attacks & checkSquares;
				data.CheckSquares[C] |= checkingMoves;
				data.AttackUnits[C] += Popcount(checkingMoves) * CheckThreatWeight;
			}
		}

		result.SetPieceEval<C, P>(mg, eg);
	}

	template<Color C>
	void EvaluateKingSafety(const Position& position, EvaluationResult& result, EvaluationData& data)
	{
		ValueType mg = 0;
		ValueType eg = 0;

		SquareIndex kingSquare = position.GetKingSquare(C);
		File kingFile = FileOf(kingSquare);
		Rank kingRank = RankOf(kingSquare);

		BitBoard safeChecks = data.CheckSquares[OtherColor(C)] & ~position.GetPieces(OtherColor(C));
		safeChecks &= ~data.AttackedBy[C][PIECE_ALL] | (data.AttackedByTwice[OtherColor(C)] & ~data.AttackedByTwice[C] & (data.AttackedBy[C][PIECE_KING] | data.AttackedBy[C][PIECE_QUEEN]));
		if (safeChecks)
		{
			data.AttackerCount[OtherColor(C)]++;
			data.AttackUnits[OtherColor(C)] += Popcount(safeChecks) * SafeCheckWeight;
		}

		ValueType danger = data.AttackUnits[OtherColor(C)] * data.AttackUnits[OtherColor(C)] * AttackerCountScaling[std::min(data.AttackerCount[OtherColor(C)], MaxAttackerCount - 1)] / MaxAttackerCount;
		if (!position.GetPieces(OtherColor(C), PIECE_QUEEN))
			danger = std::max(danger - 100, 0);
		mg -= danger;
		eg -= danger / 8;

		const BitBoard pawnMask = InFrontOrEqual<C>(kingRank);

		// King Shield
		File kingFileCenter = std::clamp(kingFile, FILE_B, FILE_G);
		for (File file = File(kingFileCenter - 1); file <= kingFileCenter + 1; file++)
		{
			BitBoard pawns = position.GetPieces(C, PIECE_PAWN) & FILE_MASKS[file] & pawnMask;
			if (pawns)
			{
				SquareIndex advancedPawn = FrontmostSquare<C>(pawns);

				// PawnShield indexed relative to black
				SquareIndex index = CreateSquare(file, RelativeRank<C>(RankOf(advancedPawn)));
				mg += PawnShield[index];
			}
			else
			{
				// Special value used when there is no pawn
				mg += PawnShield[CreateSquare(file, RANK_1)];
			}

			BitBoard enemyPawns = position.GetPieces(OtherColor(C), PIECE_PAWN) & FILE_MASKS[file] & pawnMask;
			while (enemyPawns)
			{
				SquareIndex square = PopLeastSignificantBit(enemyPawns);
				bool unopposed = pawns == ZERO_BB;
				SquareIndex index = CreateSquare(file, RelativeRank<C>(RankOf(square)));
				mg += PawnStorm[unopposed][index];
			}
		}

		mg += GetPieceSquareValue<C, PIECE_KING, MIDGAME>(kingSquare);
		eg += GetPieceSquareValue<C, PIECE_KING, ENDGAME>(kingSquare);

		result.KingSafety[C][MIDGAME] = mg;
		result.KingSafety[C][ENDGAME] = eg;
	}

	template<Color C>
	void EvaluateSpace(const Position& position, EvaluationResult& result, const EvaluationData& data)
	{
		constexpr BitBoard SpaceMask =
			C == COLOR_WHITE ? (CenterFiles & (RANK_2_BB | RANK_3_BB | RANK_4_BB))
							 : (CenterFiles & (RANK_7_BB | RANK_6_BB | RANK_5_BB));
		constexpr Direction Down = C == COLOR_WHITE ? SOUTH : NORTH;

		ValueType mg = 0;
		ValueType eg = 0;

		if (position.GetNonPawnMaterial() > 5900)
		{
			BitBoard safe = SpaceMask & ~position.GetPieces(C, PIECE_PAWN) & ~data.AttackedBy[OtherColor(C)][PIECE_PAWN];
			BitBoard behind = position.GetPieces(C, PIECE_PAWN);
			behind |= Shift<Down>(behind);
			behind |= Shift<Down>(behind);
			behind |= Shift<Down>(behind);

			int count = Popcount(safe) + Popcount(behind & safe & ~data.AttackedBy[OtherColor(C)][PIECE_ALL]);
			int weight = Popcount(position.GetPieces(C));
			mg = GetSpaceValue(weight, count);
		}

		result.Space[C][MIDGAME] = mg;
		result.Space[C][ENDGAME] = eg;
	}

	template<Color C>
	void InitEvaluationData(const Position& position, EvaluationData& data)
	{
		data.KingAttackZone[C] = GetKingAttackZone<C>(position);
		data.AttackerCount[C] = 0;
		data.AttackUnits[C] = 0;
		data.CheckSquares[C] = ZERO_BB;

		data.AttackedBy[C][PIECE_KING] = GetAttacks<PIECE_KING>(position.GetKingSquare(C));
		data.AttackedBy[C][PIECE_PAWN] = GetPawnAttacks<C>(position.GetPieces(C, PIECE_PAWN));
		data.AttackedByTwice[C] = data.AttackedBy[C][PIECE_PAWN] & data.AttackedBy[C][PIECE_KING];
		data.AttackedBy[C][PIECE_ALL] = data.AttackedBy[C][PIECE_PAWN] | data.AttackedBy[C][PIECE_KING];
	}

	template<Color C>
	void PostInitEvaluationData(const Position& position, EvaluationData& data)
	{
		data.MobilityArea[C] = ~(position.GetPieces(C, PIECE_PAWN) | data.AttackedBy[OtherColor(C)][PIECE_PAWN] | position.GetKingSquare(C));
	}

	EvaluationResult EvaluateDetailed(const Position& position)
	{
		EvaluationResult result;
		result.Stage = GameStageMax;

		EvaluationData data;
		InitEvaluationData<COLOR_WHITE>(position, data);
		InitEvaluationData<COLOR_BLACK>(position, data);
		PostInitEvaluationData<COLOR_WHITE>(position, data);
		PostInitEvaluationData<COLOR_BLACK>(position, data);

		EvaluateMaterial<COLOR_WHITE>(position, result);
		EvaluateMaterial<COLOR_BLACK>(position, result);

		EvaluatePieces<COLOR_WHITE, PIECE_KNIGHT>(position, result, data);
		EvaluatePieces<COLOR_BLACK, PIECE_KNIGHT>(position, result, data);
		EvaluatePieces<COLOR_WHITE, PIECE_BISHOP>(position, result, data);
		EvaluatePieces<COLOR_BLACK, PIECE_BISHOP>(position, result, data);
		EvaluatePieces<COLOR_WHITE, PIECE_ROOK>(position, result, data);
		EvaluatePieces<COLOR_BLACK, PIECE_ROOK>(position, result, data);
		EvaluatePieces<COLOR_WHITE, PIECE_QUEEN>(position, result, data);
		EvaluatePieces<COLOR_BLACK, PIECE_QUEEN>(position, result, data);

		EvaluatePawns<COLOR_WHITE>(position, result, data);
		EvaluatePawns<COLOR_BLACK>(position, result, data);

		EvaluateKingSafety<COLOR_WHITE>(position, result, data);
		EvaluateKingSafety<COLOR_BLACK>(position, result, data);

		EvaluateSpace<COLOR_WHITE>(position, result, data);
		EvaluateSpace<COLOR_BLACK>(position, result, data);

		BitBoard allPawns = position.GetPieces(PIECE_PAWN);
		result.Initiative =
			InitiativeBonuses[0] * Popcount(allPawns) +
			InitiativeBonuses[1] * ((allPawns & QueensideMask) && (allPawns & KingsideMask)) +
			InitiativeBonuses[2] * (Popcount(position.GetPieces() & ~allPawns) == 2) -
			InitiativeBonuses[3];

		return result;
	}

	ValueType Evaluate(const Position& position)
	{
		ValueType eval;
		if (position.IsNetworkEnabled())
			eval = NNUE::EvaluateNNUE(position, true);
		else
			eval = EvaluateDetailed(position).Result(position.ColorToMove);
		eval = eval * (100 - position.HalfTurnsSinceCaptureOrPush) / 100;
		return eval;
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
		format += "          Space | " + FORMAT_TABLE_ROW(result.Space, SCORE_LENGTH) + '\n';
		format += " ---------------+---------------+---------------+--------------\n";
		format += "          Total | " + FORMAT_TABLE_ROW(totals, SCORE_LENGTH) + "\n";
		format += "\n";
		format += "Game stage: " + std::to_string(result.Stage) + " / " + std::to_string(GameStageMax) + '\n';
		format += "Total evaluation: " + std::to_string(result.Result(COLOR_WHITE)) + " (white side)";
		return format;
#undef FORMAT_TABLE_ROW
	}

	std::string FormatNNUEEvaluation(Position& position)
	{
		auto formatCp = [](ValueType cp)
		{
			std::string buffer = "     ";
			buffer[0] = (cp < 0 ? '-' : cp > 0 ? '+' : ' ');
			cp = std::abs(cp * 100 / 130);

			if (cp >= 10000)
			{
				buffer[1] = '0' + cp / 10000; cp %= 10000;
				buffer[2] = '0' + cp / 1000; cp %= 1000;
				buffer[3] = '0' + cp / 100; cp %= 100;
				buffer[4] = ' ';
			}
			else if (cp >= 1000)
			{
				buffer[1] = '0' + cp / 1000; cp %= 1000;
				buffer[2] = '0' + cp / 100; cp %= 100;
				buffer[3] = '.';
				buffer[4] = '0' + cp / 10;
			}
			else
			{
				buffer[1] = '0' + cp / 100; cp %= 100;
				buffer[2] = '.';
				buffer[3] = '0' + cp / 10; cp %= 10;
				buffer[4] = '0' + cp / 1;
			}
			return buffer;
		};

		ValueType baseEval = Evaluate(position);

		std::stringstream ss;
		for (Rank rank = RANK_8; rank >= RANK_1; rank--)
		{
			for (File file = FILE_A; file < FILE_MAX; file++)
			{
				ss << "+-------";
			}
			ss << "+\n";
			for (File file = FILE_A; file < FILE_MAX; file++)
			{
				ColorPiece piece = position.GetPieceOnSquare(CreateSquare(file, rank));
				char c = piece == COLOR_PIECE_NONE ? ' ' : UCI::PieceToString(TypeOf(piece), ColorOf(piece));
				ss << "|   " << c << "   ";
			}
			ss << "|\n";
			for (File file = FILE_A; file < FILE_MAX; file++)
			{
				ss << "| ";
				SquareIndex square = CreateSquare(file, rank);
				ColorPiece piece = position.GetPieceOnSquare(square);
				if (piece != COLOR_PIECE_NONE && TypeOf(piece) != PIECE_KING)
				{
					position.RemovePiece(square);
					position.GetState()->Accumulator.computed[COLOR_WHITE] = false;
					position.GetState()->Accumulator.computed[COLOR_BLACK] = false;

					ValueType eval = Evaluate(position);
					eval = position.ColorToMove == COLOR_WHITE ? eval : -eval;
					ValueType pieceValue = baseEval - eval;

					position.AddPiece(piece, square);
					position.GetState()->Accumulator.computed[COLOR_WHITE] = false;
					position.GetState()->Accumulator.computed[COLOR_BLACK] = false;
					ss << formatCp(pieceValue);
				}
				else
				{
					ss << "     ";
				}

				ss << " ";
			}
			ss << "|\n";
		}
		for (File file = FILE_A; file < FILE_MAX; file++)
		{
			ss << "+-------";
		}
		ss << "+";
		return ss.str();
	}
}
