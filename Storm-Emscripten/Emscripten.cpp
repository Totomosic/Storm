#include <string>
#include <vector>
#include <iostream>
#include <thread>
#include "Storm.h"
using namespace Storm;

#include <xmmintrin.h>
#include <emscripten/bind.h>
using namespace emscripten;

void PrintPosition(const Position& position)
{
	std::cout << position << std::endl;
}

Move SearchBestMoveTime(Search& search, const Position& position, int milliseconds)
{
	SearchLimits limits;
	limits.Milliseconds = milliseconds;

	return search.SearchBestMove(position, limits).Move;
}

Move SearchBestMoveDepth(Search& search, const Position& position, int depth)
{
	SearchLimits limits;
	limits.Depth = depth;

	return search.SearchBestMove(position, limits).Move;
}

std::vector<Move> GenerateLegalMoves(const Position& position)
{
	Move buffer[MAX_MOVES];
	MoveList moves(buffer);

	moves.FillLegal<ALL>(position);
	return { moves.begin(), moves.end() };
}

Color PositionGetColorToMove(const Position& position)
{
	return position.ColorToMove;
}

void PositionSetColorToMove(Position& position, Color ctm)
{
	position.ColorToMove = ctm;
}

SquareIndex PositionGetEnpassant(const Position& position)
{
	return position.EnpassantSquare;
}

void PositionSetEnpassant(Position& position, SquareIndex enp)
{
	position.EnpassantSquare = enp;
}

SquareIndex PositionEnpassantCaptureSquare(const Position& position)
{
	return GetEnpassantSquare(position.EnpassantSquare, position.ColorToMove);
}

std::string PositionHash(const Position& position)
{
	return std::to_string(position.Hash.Hash);
}

int PositionHalfTurnsSinceCaptureOrPush(const Position& position)
{
	return position.HalfTurnsSinceCaptureOrPush;
}

int PositionTotalTurns(const Position& position)
{
	return position.TotalTurns;
}

bool PositionCanKingsideCastle(const Position& position, Color color)
{
	return position.Colors[color].CastleKingSide;
}

bool PositionCanQueensideCastle(const Position& position, Color color)
{
	return position.Colors[color].CastleQueenSide;
}

void PositionApplyMove(Position& position, Move move, StateInfo& st, UndoInfo& undo)
{
	position.ApplyMove(move, st, &undo);
}

void PositionReset(Position& position, StateInfo& st)
{
	position.Reset(&st);
}

Position PositionClone(const Position& position)
{
	return position;
}

void SearchSetLevel(Search& search, int level)
{
	SearchSettings settings = search.GetSettings();
	settings.SkillLevel = level;
	search.SetSettings(settings);
}

void SearchSetMultiPV(Search& search, int multiPv)
{
	SearchSettings settings = search.GetSettings();
	settings.MultiPv = multiPv;
	search.SetSettings(settings);
}

void SearchSetThreads(Search& search, int threads)
{
	SearchSettings settings = search.GetSettings();
	settings.Threads = threads;
	search.SetSettings(settings);
}

void SearchPonder(Search& search, const Position& position, emscripten::val callback)
{
	SearchLimits limits;
	search.Ponder(position, limits, [callback](const SearchResult& result)
	{
		callback(result);
	});
}

Move CreateMoveHelper(const Position& position, SquareIndex from, SquareIndex to, Piece promotion)
{
	Piece movingPiece = TypeOf(position.GetPieceOnSquare(from));
	if (movingPiece == PIECE_KING && FileOf(from) == FILE_E && (FileOf(to) == FILE_C || FileOf(to) == FILE_G))
		return CreateMove(from, to, CASTLE);
	if (movingPiece == PIECE_PAWN)
	{
		if (position.ColorToMove == COLOR_WHITE && RankOf(to) == RANK_8)
			return CreateMove(from, to, promotion);
		else if (position.ColorToMove == COLOR_BLACK && RankOf(to) == RANK_1)
			return CreateMove(from, to, promotion);
	}
	return CreateMove(from, to);
}

enum EvaluationType
{
	EVAL_MATERIAL,
	EVAL_PAWNS,
	EVAL_KNIGHTS,
	EVAL_BISHOPS,
	EVAL_ROOKS,
	EVAL_QUEENS,
	EVAL_KING_SAFETY,
	EVAL_SPACE,
};

ValueType EvaluationResultGetColorComponent(const EvaluationResult& result, EvaluationType type, GameStage stage, Color color)
{
	switch (type)
	{
	case EVAL_MATERIAL:
		return result.Material[color][stage];
	case EVAL_PAWNS:
		return result.Pawns[color][stage];
	case EVAL_KNIGHTS:
		return result.Knights[color][stage];
	case EVAL_BISHOPS:
		return result.Bishops[color][stage];
	case EVAL_ROOKS:
		return result.Rooks[color][stage];
	case EVAL_QUEENS:
		return result.Queens[color][stage];
	case EVAL_KING_SAFETY:
		return result.KingSafety[color][stage];
	case EVAL_SPACE:
		return result.Space[color][stage];
	}
	return 0;
}

ValueType EvaluationResultGetComponent(const EvaluationResult& result, EvaluationType type, GameStage stage)
{
	return EvaluationResultGetColorComponent(result, type, stage, COLOR_WHITE) - EvaluationResultGetColorComponent(result, type, stage, COLOR_BLACK);
}

int EvaluationResultGetStage(const EvaluationResult& result)
{
	return result.Stage;
}

EMSCRIPTEN_BINDINGS(Storm) {
	register_vector<Move>("MoveList");
	register_vector<PGNMatch>("MatchList");

	constant("SQUARE_MAX", (int)SQUARE_MAX);
	constant("VALUE_MATE", VALUE_MATE);
	constant("VALUE_NONE", VALUE_NONE);
	constant("VALUE_DRAW", VALUE_DRAW);
	constant("MOVE_NONE", MOVE_NONE);
	constant("GAME_STAGE_MAX", GameStageMax);

	function("Init", &Init);
	function("IsNNUEAvailable", &IsNNUEAvailable);
	function("PrintPosition", &PrintPosition);
	function("OtherColor", &OtherColor);
	function("CreateStartingPosition", &CreateStartingPosition);
	function("CreatePositionFromFEN", &CreatePositionFromFEN);
	function("GetFENFromPosition", &GetFENFromPosition);
	function("CreateNormalMove", select_overload<Move(SquareIndex, SquareIndex)>(&CreateMove));
	function("CreateMove", &CreateMoveHelper);
	function("GenerateLegalMoves", &GenerateLegalMoves);

	function("FileOf", &FileOf);
	function("RankOf", &RankOf);
	function("TypeOf", &TypeOf);
	function("ColorOf", &ColorOf);

	function("GetFromSquare", &GetFromSquare);
	function("GetToSquare", &GetToSquare);
	function("GetMoveType", &GetMoveType);
	function("GetPromotionPiece", &GetPromotionPiece);

	function("CreateSquare", &CreateSquare);

	enum_<EvaluationType>("Evaluation")
		.value("Material", EVAL_MATERIAL)
		.value("Pawns", EVAL_PAWNS)
		.value("Knights", EVAL_KNIGHTS)
		.value("Bishops", EVAL_BISHOPS)
		.value("Rooks", EVAL_ROOKS)
		.value("Queens", EVAL_QUEENS)
		.value("KingSafety", EVAL_KING_SAFETY)
		.value("Space", EVAL_SPACE);

	class_<EvaluationResult>("EvaluationResult")
		.property("Stage", &EvaluationResultGetStage)
		.function("GetComponent", &EvaluationResultGetComponent)
		.function("GetColorComponent", &EvaluationResultGetColorComponent)
		.function("GetMidgameTotal", &EvaluationResult::GetStageTotal<MIDGAME>)
		.function("GetEndgameTotal", &EvaluationResult::GetStageTotal<ENDGAME>)
		.function("Result", &EvaluationResult::Result);

	function("EvaluateDetailed", &EvaluateDetailed);
	function("Evaluate", &Evaluate);

	enum_<GameStage>("GameStage")
		.value("Midgame", MIDGAME)
		.value("Endgame", ENDGAME);
	enum_<Color>("Color")
		.value("White", COLOR_WHITE)
		.value("Black", COLOR_BLACK);
	enum_<Piece>("Piece")
		.value("Pawn", PIECE_PAWN)
		.value("Knight", PIECE_KNIGHT)
		.value("Bishop", PIECE_BISHOP)
		.value("Rook", PIECE_ROOK)
		.value("Queen", PIECE_QUEEN)
		.value("King", PIECE_KING)
		.value("None", PIECE_NONE)
		.value("All", PIECE_ALL);
	enum_<ColorPiece>("ColorPiece")
		.value("None", COLOR_PIECE_NONE)
		.value("WhitePawn", WHITE_PAWN)
		.value("WhiteKnight", WHITE_KNIGHT)
		.value("WhiteBishop", WHITE_BISHOP)
		.value("WhiteRook", WHITE_ROOK)
		.value("WhiteQueen", WHITE_QUEEN)
		.value("WhiteKing", WHITE_KING)
		.value("BlackPawn", BLACK_PAWN)
		.value("BlackKnight", BLACK_KNIGHT)
		.value("BlackBishop", BLACK_BISHOP)
		.value("BlackRook", BLACK_ROOK)
		.value("BlackQueen", BLACK_QUEEN)
		.value("BlackKing", BLACK_KING);
	enum_<MoveType>("MoveType")
		.value("Castle", CASTLE)
		.value("Promotion", PROMOTION)
		.value("Normal", NORMAL);
	enum_<File>("File")
		.value("FILE_A", FILE_A)
		.value("FILE_B", FILE_B)
		.value("FILE_C", FILE_C)
		.value("FILE_D", FILE_D)
		.value("FILE_E", FILE_E)
		.value("FILE_F", FILE_F)
		.value("FILE_G", FILE_G)
		.value("FILE_H", FILE_H)
		.value("FILE_MAX", FILE_MAX);
	enum_<Rank>("Rank")
		.value("RANK_1", RANK_1)
		.value("RANK_2", RANK_2)
		.value("RANK_3", RANK_3)
		.value("RANK_4", RANK_4)
		.value("RANK_5", RANK_5)
		.value("RANK_6", RANK_6)
		.value("RANK_7", RANK_7)
		.value("RANK_8", RANK_8)
		.value("RANK_MAX", RANK_MAX);
	enum_<SquareIndex>("SquareIndex")
		.value("a1", a1)
		.value("b1", b1)
		.value("c1", c1)
		.value("d1", d1)
		.value("e1", e1)
		.value("f1", f1)
		.value("g1", g1)
		.value("h1", h1)

		.value("a2", a2)
		.value("b2", b2)
		.value("c2", c2)
		.value("d2", d2)
		.value("e2", e2)
		.value("f2", f2)
		.value("g2", g2)
		.value("h2", h2)

		.value("a3", a3)
		.value("b3", b3)
		.value("c3", c3)
		.value("d3", d3)
		.value("e3", e3)
		.value("f3", f3)
		.value("g3", g3)
		.value("h3", h3)

		.value("a4", a4)
		.value("b4", b4)
		.value("c4", c4)
		.value("d4", d4)
		.value("e4", e4)
		.value("f4", f4)
		.value("g4", g4)
		.value("h4", h4)

		.value("a5", a5)
		.value("b5", b5)
		.value("c5", c5)
		.value("d5", d5)
		.value("e5", e5)
		.value("f5", f5)
		.value("g5", g5)
		.value("h5", h5)

		.value("a6", a6)
		.value("b6", b6)
		.value("c6", c6)
		.value("d6", d6)
		.value("e6", e6)
		.value("f6", f6)
		.value("g6", g6)
		.value("h6", h6)

		.value("a7", a7)
		.value("b7", b7)
		.value("c7", c7)
		.value("d7", d7)
		.value("e7", e7)
		.value("f7", f7)
		.value("g7", g7)
		.value("h7", h7)

		.value("a8", a8)
		.value("b8", b8)
		.value("c8", c8)
		.value("d8", d8)
		.value("e8", e8)
		.value("f8", f8)
		.value("g8", g8)
		.value("h8", h8)
		
		.value("Invalid", SQUARE_INVALID);

	class_<StateInfo>("StateInfo")
		.constructor<>();

	class_<UndoInfo>("UndoInfo")
		.constructor<>();
	class_<UCI>("UCI")
		.class_function("FormatMove", &UCI::FormatMove)
		.class_function("CreateMoveFromString", &UCI::CreateMoveFromString);
	class_<PGN>("PGN")
		.class_function("FormatMove", &PGN::FormatMove)
		.class_function("CreateMoveFromString", &PGN::CreateMoveFromString)
		.class_function("ReadFromString", &PGN::ReadFromString);
	class_<Position>("Position")
		.property("ColorToMove", &PositionGetColorToMove, &PositionSetColorToMove)
		.property("EnpassantSquare", &PositionGetEnpassant, &PositionSetEnpassant)
		.property("HalfTurnsSinceCaptureOrPush", &PositionHalfTurnsSinceCaptureOrPush)
		.property("TotalTurns", &PositionTotalTurns)
		.property("Hash", &PositionHash)
		.function("GetPieceOnSquare", &Position::GetPieceOnSquare)
		.function("GetKingSquare", &Position::GetKingSquare)
		.function("GetEnpassantCaptureSquare", &PositionEnpassantCaptureSquare)
		.function("InCheck", &Position::InCheck)
		.function("GetColorOnSquare", &Position::GetColorOnSquare)
		.function("IsCapture", &Position::IsCapture)
		.function("IsEnpassant", &Position::IsEnpassant)
		.function("CanKingsideCastle", &PositionCanKingsideCastle)
		.function("CanQueensideCastle", &PositionCanQueensideCastle)
		.function("ApplyMove", &PositionApplyMove)
		.function("Reset", &PositionReset)
		.function("SetNetworkEnabled", &Position::SetNetworkEnabled)
		.function("DeepClone", &PositionClone);
	value_object<SearchLimits>("SearchLimits")
		.field("Infinite", &SearchLimits::Infinite)
		.field("Depth", &SearchLimits::Depth)
		.field("Nodes", &SearchLimits::Nodes)
		.field("Milliseconds", &SearchLimits::Milliseconds);
	value_object<SearchResult>("SearchResult")
		.field("PV", &SearchResult::PV)
		.field("Score", &SearchResult::Score)
		.field("BestMove", &SearchResult::BestMove)
		.field("PVIndex", &SearchResult::PVIndex)
		.field("Depth", &SearchResult::Depth)
		.field("SelDepth", &SearchResult::SelDepth);
	class_<Search>("Search")
		.constructor<>()
		.function("SetLevel", &SearchSetLevel)
		.function("SetMultiPV", &SearchSetMultiPV)
		.function("SetLogging", &Search::SetLogging)
		.function("SetThreads", &SearchSetThreads)
		.function("Perft", &Search::Perft)
		.function("SearchMoveTime", &SearchBestMoveTime)
		.function("SearchDepth", &SearchBestMoveDepth)
		.function("Ponder", &SearchPonder);
	value_object<PGNMatch>("PGNMatch")
		.field("InitialPosition", &PGNMatch::InitialPosition)
		.field("Moves", &PGNMatch::Moves);
}
