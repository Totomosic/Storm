#pragma once
#include <string>

#include "BitBoard.h"
#include "ZobristHash.h"
#include "Move.h"

namespace Storm
{

	struct UndoInfo
	{
	public:
		SquareIndex EnpassantSquare;
	};

	class STORM_API Position
	{
	public:
		struct STORM_API PositionCache
		{
		public:
			BitBoard ColorPieces[COLOR_MAX];
			BitBoard PiecesByType[PIECE_MAX];
			BitBoard AllPieces;

			SquareIndex KingSquare[COLOR_MAX];
			BitBoard CheckedBy[COLOR_MAX];
			// Squares that if a piece moves to will give check
			BitBoard CheckSquares[COLOR_MAX][PIECE_COUNT];

			BitBoard BlockersForKing[COLOR_MAX];
			BitBoard Pinners[COLOR_MAX];
			Piece PieceOnSquare[SQUARE_MAX];

			ValueType NonPawnMaterial[COLOR_MAX];
		};

		struct STORM_API ColorData
		{
		public:
			BitBoard Pieces[PIECE_MAX] = { ZERO_BB };
			bool CastleKingSide = false;
			bool CastleQueenSide = false;
		};
	public:
		ColorData Colors[COLOR_MAX];
		Color ColorToMove = COLOR_WHITE;
		int HalfTurnsSinceCaptureOrPush = 0;
		int TotalTurns = 0;
		SquareIndex EnpassantSquare = SQUARE_INVALID;
		ZobristHash Hash;

		PositionCache Cache;

	public:
		void Initialize();

		inline BitBoard GetPieces() const { return Cache.AllPieces; }
		inline BitBoard GetPieces(Piece p0) const { return GetPieces(COLOR_WHITE, p0) | GetPieces(COLOR_BLACK, p0); }
		inline BitBoard GetPieces(Piece p0, Piece p1) const { return GetPieces(p0) | GetPieces(p1); }
		inline BitBoard GetPieces(Piece p0, Piece p1, Piece p2) const { return GetPieces(p0, p1) | GetPieces(p2); }
		inline BitBoard GetPieces(Color color) const { return Cache.ColorPieces[color]; }
		inline BitBoard GetPieces(Color color, Piece p0) const { return Colors[color].Pieces[p0]; }
		inline BitBoard GetPieces(Color color, Piece p0, Piece p1) const { return GetPieces(color, p0) | GetPieces(color, p1); }
		inline BitBoard GetPieces(Color color, Piece p0, Piece p1, Piece p2) const { return GetPieces(color, p0, p1) | GetPieces(color, p2); }
		inline SquareIndex GetKingSquare(Color color) const { return Cache.KingSquare[color]; }
		inline Piece GetPieceOnSquare(SquareIndex square) const { return Cache.PieceOnSquare[square]; }
		inline Color GetColorAt(SquareIndex square) const { return (GetPieces(COLOR_WHITE) & square) ? COLOR_WHITE : COLOR_BLACK; }
		inline bool SquareOccupied(SquareIndex square) const { return GetPieceOnSquare(square) != PIECE_NONE; }
		inline BitBoard GetBlockersForKing(Color color) const { return Cache.BlockersForKing[color]; }

		inline bool InCheck() const { return Cache.CheckedBy[ColorToMove] != ZERO_BB; }

		inline ValueType GetNonPawnMaterial(Color color) const { return Cache.NonPawnMaterial[color]; }
		inline ValueType GetNonPawnMaterial() const { return GetNonPawnMaterial(COLOR_WHITE) + GetNonPawnMaterial(COLOR_BLACK); }
		inline int GetTotalHalfMoves() const { return 2 * TotalTurns + (ColorToMove == COLOR_BLACK); }

		inline Piece GetMovingPiece(Move move) const { return GetPieceOnSquare(GetFromSquare(move)); }
		inline Piece GetCapturedPiece(Move move) const { return GetPieceOnSquare(GetToSquare(move)); }

		bool GivesCheck(Move move) const;
		void ApplyMove(Move move, UndoInfo* undo, bool givesCheck);
		void ApplyMove(Move move, UndoInfo* undo);
		void UndoMove(const UndoInfo& undo);
		bool IsLegal(Move move) const;
		BitBoard GetSliderBlockers(BitBoard sliders, SquareIndex toSquare, BitBoard* pinners) const;
		BitBoard GetAttackersTo(SquareIndex square, Color by, BitBoard blockers) const;

	private:
		void MovePiece(Color color, Piece piece, SquareIndex from, SquareIndex to);
		void AddPiece(Color color, Piece piece, SquareIndex square);
		void RemovePiece(Color color, Piece piece, SquareIndex square);

		void UpdateCheckInfo(Color color);
	};

	Position CreateStartingPosition();
	Position CreatePositionFromFEN(const std::string& fen);
	std::string GetFENFromPosition(const Position& position);

	std::string FormatPosition(const Position& position);
	std::ostream& operator<<(std::ostream& stream, const Position& position);

}
