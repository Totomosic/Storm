#pragma once
#include <string>

#include "Bitboard.h"
#include "ZobristHash.h"
#include "Move.h"

#include "nnue/nnue_accumulator.h"

namespace Storm
{

	// Keep track of what a move changes on the board (used by NNUE)
	struct STORM_API DirtyPiece
	{
	public:
		// Number of changed pieces
		int dirty_num;

		// Max 3 pieces can change in one move. A promotion with capture moves
		// both the pawn and the captured piece to SQ_NONE and the piece promoted
		// to from SQ_NONE to the capture square.
		ColorPiece piece[3];

		// From and to squares, which may be SQ_NONE
		SquareIndex from[3];
		SquareIndex to[3];
	};

	struct STORM_API StateInfo
	{
	public:
		Storm::NNUE::Accumulator Accumulator;
		Storm::DirtyPiece DirtyPiece;
		StateInfo* Previous;
	};

	struct STORM_API UndoInfo
	{
	public:
		SquareIndex EnpassantSquare;
		Piece CapturedPiece;
		int HalfTurnsSinceCaptureOrPush;
		bool CastlingRights[4];
		BitBoard CheckedBy;
		BitBoard BlockersForKing[COLOR_MAX];
		BitBoard Pinners[COLOR_MAX];
		BitBoard CheckSquares[PIECE_COUNT];
	};

	class STORM_API Position
	{
	public:
		struct STORM_API PositionCache
		{
		public:
			// All Pieces owned by a certain color
			BitBoard ColorPieces[COLOR_MAX];
			// All Peices of a given type
			BitBoard PiecesByType[PIECE_MAX];
			// All pieces on board
			BitBoard AllPieces;

			// King square of given color
			SquareIndex KingSquare[COLOR_MAX];
			// All enemy pieces that attack the king square of the ColorToMove.
			BitBoard CheckedBy;
			// Squares that if a piece moves to will give check
			BitBoard CheckSquares[COLOR_MAX][PIECE_COUNT];

			// Pieces (of both colors) that if moved will cause our king to be attacked (unless moving in a line between attacker and king)
			// BlockersForKing[COLOR_WHITE] contains pieces that block black rooks, queens and bishops from our king
			BitBoard BlockersForKing[COLOR_MAX];
			// Pieces that attack a piece that blocks the enemy king
			BitBoard Pinners[COLOR_MAX];
			// Piece type on squares - COLOR_PIECE_NONE if no piece on square
			ColorPiece PieceOnSquare[SQUARE_MAX];

			// Midgame Non pawn material
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

	private:
		bool m_UseNetwork = false;
		StateInfo* m_StateInfo;

	public:
		void Initialize();

		inline void SetNetworkEnabled(bool enabled)
		{
			m_UseNetwork = enabled;
		}

		void Reset(StateInfo* st);
		inline StateInfo* GetState() const { return m_StateInfo; }

		inline BitBoard GetPieces() const { return Cache.AllPieces; }
		inline BitBoard GetPieces(Piece p0) const { return GetPieces(COLOR_WHITE, p0) | GetPieces(COLOR_BLACK, p0); }
		inline BitBoard GetPieces(Piece p0, Piece p1) const { return GetPieces(p0) | GetPieces(p1); }
		inline BitBoard GetPieces(Piece p0, Piece p1, Piece p2) const { return GetPieces(p0, p1) | GetPieces(p2); }
		inline BitBoard GetPieces(Color color) const { return Cache.ColorPieces[color]; }
		inline BitBoard GetPieces(Color color, Piece p0) const { return Colors[color].Pieces[p0]; }
		inline BitBoard GetPieces(Color color, Piece p0, Piece p1) const { return GetPieces(color, p0) | GetPieces(color, p1); }
		inline BitBoard GetPieces(Color color, Piece p0, Piece p1, Piece p2) const { return GetPieces(color, p0, p1) | GetPieces(color, p2); }
		inline SquareIndex GetKingSquare(Color color) const { return Cache.KingSquare[color]; }
		inline ColorPiece GetPieceOnSquare(SquareIndex square) const { return Cache.PieceOnSquare[square]; }
		inline bool SquareOccupied(SquareIndex square) const { return GetPieceOnSquare(square) != COLOR_PIECE_NONE; }
		inline BitBoard GetBlockersForKing(Color color) const { return Cache.BlockersForKing[color]; }
		inline Color GetColorOnSquare(SquareIndex square) const { return (GetPieces(COLOR_WHITE) & square) ? COLOR_WHITE : COLOR_BLACK; }

		inline bool InCheck() const { return Cache.CheckedBy != ZERO_BB; }
		inline BitBoard GetCheckers() const { return Cache.CheckedBy; };

		inline ValueType GetNonPawnMaterial(Color color) const { return Cache.NonPawnMaterial[color]; }
		inline ValueType GetNonPawnMaterial() const { return GetNonPawnMaterial(COLOR_WHITE) + GetNonPawnMaterial(COLOR_BLACK); }
		inline int GetTotalHalfMoves() const { return 2 * TotalTurns + (ColorToMove == COLOR_BLACK); }

		inline Piece GetMovingPiece(Move move) const { return TypeOf(GetPieceOnSquare(GetFromSquare(move))); }
		inline Piece GetCapturedPiece(Move move) const { return TypeOf(GetPieceOnSquare(GetToSquare(move))); }
		inline bool IsCapture(Move move) const { return GetPieceOnSquare(GetToSquare(move)) != COLOR_PIECE_NONE || IsEnpassant(move); }
		inline bool IsEnpassant(Move move) const { return TypeOf(GetPieceOnSquare(GetFromSquare(move))) == PIECE_PAWN && GetToSquare(move) == EnpassantSquare; }

		bool GivesCheck(Move move) const;
		void ApplyMove(Move move, StateInfo& st, UndoInfo* undo, bool givesCheck);
		void ApplyMove(Move move, StateInfo& st, UndoInfo* undo);
		void UndoMove(Move move, const UndoInfo& undo);
		void ApplyNullMove(StateInfo& st, UndoInfo* undo);
		void UndoNullMove(const UndoInfo& undo);
		bool IsPseudoLegal(Move move) const;
		bool IsLegal(Move move) const;
		BitBoard GetSliderBlockers(BitBoard sliders, SquareIndex toSquare, BitBoard* pinners) const;
		BitBoard GetAttackersTo(SquareIndex square, Color by, BitBoard blockers) const;
		BitBoard GetAttackersTo(SquareIndex square, BitBoard blockers) const;

		bool SeeGE(Move move, ValueType threshold = 0) const;

		inline bool IsNetworkAvailable() const { return true; }
		inline bool IsNetworkEnabled() const { return m_UseNetwork && IsNetworkAvailable(); }

		void AddPiece(ColorPiece piece, SquareIndex square);
		void RemovePiece(SquareIndex square);

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
