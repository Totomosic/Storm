#include "Position.h"
#include "Format.h"
#include "EvalConstants.h"
#include "Attacks.h"

namespace Storm
{

	void Position::Initialize()
	{
		Cache.NonPawnMaterial[COLOR_WHITE] = 0;
		Cache.NonPawnMaterial[COLOR_BLACK] = 0;

		for (SquareIndex sq = a1; sq < SQUARE_MAX; sq++)
			Cache.PieceOnSquare[sq] = COLOR_PIECE_NONE;

		Cache.ColorPieces[COLOR_WHITE] = ZERO_BB;
		Cache.ColorPieces[COLOR_BLACK] = ZERO_BB;

		for (Piece piece = PIECE_START; piece < PIECE_MAX; piece++)
		{
			Cache.ColorPieces[COLOR_WHITE] |= Colors[COLOR_WHITE].Pieces[piece];
			Cache.ColorPieces[COLOR_BLACK] |= Colors[COLOR_BLACK].Pieces[piece];
		}

		Cache.AllPieces = GetPieces(COLOR_WHITE) | GetPieces(COLOR_BLACK);

		for (Piece piece = PIECE_START; piece < PIECE_MAX; piece++)
		{
			Cache.PiecesByType[piece] = GetPieces(piece);
			if (piece != PIECE_PAWN && piece != PIECE_KING)
			{
				Cache.NonPawnMaterial[COLOR_WHITE] += GetPieceValueMg(piece) * Popcount(GetPieces(COLOR_WHITE, piece));
				Cache.NonPawnMaterial[COLOR_BLACK] += GetPieceValueMg(piece) * Popcount(GetPieces(COLOR_BLACK, piece));
			}
			BitBoard pieces = GetPieces(piece);
			while (pieces)
			{
				SquareIndex square = PopLeastSignificantBit(pieces);
				if (Cache.ColorPieces[COLOR_WHITE] & square)
					Cache.PieceOnSquare[square] = CreatePiece(piece, COLOR_WHITE);
				else
					Cache.PieceOnSquare[square] = CreatePiece(piece, COLOR_BLACK);
			}
		}

		Cache.KingSquare[COLOR_WHITE] = LeastSignificantBit(GetPieces(COLOR_WHITE, PIECE_KING));
		Cache.KingSquare[COLOR_BLACK] = MostSignificantBit(GetPieces(COLOR_BLACK, PIECE_KING));
		STORM_ASSERT(Cache.KingSquare[COLOR_WHITE] != SQUARE_INVALID && Cache.KingSquare[COLOR_BLACK] != SQUARE_INVALID, "Couldn't find king");

		Cache.CheckedBy = GetAttackersTo(GetKingSquare(ColorToMove), OtherColor(ColorToMove), GetPieces());

		UpdateCheckInfo(COLOR_WHITE);
		UpdateCheckInfo(COLOR_BLACK);

		Hash.SetFromPosition(*this);
	}

	void Position::ApplyMove(Move move, UndoInfo* undo)
	{
		return ApplyMove(move, undo, GivesCheck(move));
	}

	void Position::ApplyNullMove()
	{
		STORM_ASSERT(!GetCheckers(), "Cannot be in check");
		if (EnpassantSquare != SQUARE_INVALID)
		{
			Hash.RemoveEnPassant(FileOf(EnpassantSquare));
			EnpassantSquare = SQUARE_INVALID;
		}
		Cache.CheckedBy = ZERO_BB;
		UpdateCheckInfo(ColorToMove);

		HalfTurnsSinceCaptureOrPush++;

		if (ColorToMove == COLOR_BLACK)
			TotalTurns++;

		ColorToMove = OtherColor(ColorToMove);
		Hash.FlipTeamToPlay();
	}

	bool Position::IsPseudoLegal(Move move) const
	{
		return true;
	}

	void Position::ApplyMove(Move move, UndoInfo* undo, bool givesCheck)
	{
		STORM_ASSERT(ValidMove(move), "Invalid move");
		const SquareIndex fromSquare = GetFromSquare(move);
		const SquareIndex toSquare = GetToSquare(move);
		const Rank fromRank = RankOf(fromSquare);
		const Rank toRank = RankOf(toSquare);
		const File fromFile = FileOf(fromSquare);
		const File toFile = FileOf(toSquare);
		const Piece movingPiece = GetMovingPiece(move);
		STORM_ASSERT(movingPiece != PIECE_NONE, "No piece on square");
		const Piece capturedPiece = GetCapturedPiece(move);
		const bool isPromotion = GetMoveType(move) == PROMOTION;
		const bool isCapture = capturedPiece != PIECE_NONE;
		const bool isCastle = GetMoveType(move) == CASTLE;
		const bool isEnpassant = movingPiece == PIECE_PAWN && toSquare == EnpassantSquare;
		const Color otherColor = OtherColor(ColorToMove);
		const Rank otherBackRank = otherColor == COLOR_WHITE ? RANK_1 : RANK_8;
		const Rank castleRank = ColorToMove == COLOR_WHITE ? RANK_1 : RANK_8;

		if (EnpassantSquare != SQUARE_INVALID)
		{
			Hash.RemoveEnPassant(FileOf(EnpassantSquare));
			EnpassantSquare = SQUARE_INVALID;
		}

		if (isCapture && isPromotion)
		{
			const Piece promotionPiece = GetPromotionPiece(move);
			RemovePiece(otherColor, capturedPiece, toSquare);
			RemovePiece(ColorToMove, movingPiece, fromSquare);
			AddPiece(ColorToMove, promotionPiece, toSquare);
			if (capturedPiece == PIECE_ROOK && toRank == otherBackRank)
			{
				if (toFile == FILE_A && Colors[otherColor].CastleQueenSide)
				{
					Colors[otherColor].CastleQueenSide = false;
					Hash.RemoveCastleQueenside(otherColor);
				}
				else if (toFile == FILE_H && Colors[otherColor].CastleKingSide)
				{
					Colors[otherColor].CastleKingSide = false;
					Hash.RemoveCastleKingside(otherColor);
				}
			}
		}
		else if (isCapture)
		{
			RemovePiece(otherColor, capturedPiece, toSquare);
			MovePiece(ColorToMove, movingPiece, fromSquare, toSquare);
			if (capturedPiece == PIECE_ROOK && toRank == otherBackRank)
			{
				if (toFile == FILE_A && Colors[otherColor].CastleQueenSide)
				{
					Colors[otherColor].CastleQueenSide = false;
					Hash.RemoveCastleQueenside(otherColor);
				}
				else if (toFile == FILE_H && Colors[otherColor].CastleKingSide)
				{
					Colors[otherColor].CastleKingSide = false;
					Hash.RemoveCastleKingside(otherColor);
				}
			}
		}
		else if (isPromotion)
		{
			const Piece promotionPiece = GetPromotionPiece(move);
			RemovePiece(ColorToMove, movingPiece, fromSquare);
			AddPiece(ColorToMove, promotionPiece, toSquare);
		}
		else if (movingPiece == PIECE_PAWN && abs(toRank - fromRank) == 2)
		{
			MovePiece(ColorToMove, movingPiece, fromSquare, toSquare);
			EnpassantSquare = GetEnpassantSquare(toSquare, ColorToMove);
			Hash.AddEnPassant(FileOf(EnpassantSquare));
		}
		else if (isEnpassant)
		{
			RemovePiece(otherColor, PIECE_PAWN, GetEnpassantSquare(toSquare, ColorToMove));
			MovePiece(ColorToMove, movingPiece, fromSquare, toSquare);
		}
		else if (isCastle)
		{
			MovePiece(ColorToMove, movingPiece, fromSquare, toSquare);
			if (toFile == FILE_G)
			{
				// Kingside castle
				MovePiece(ColorToMove, PIECE_ROOK, CreateSquare(FILE_H, toRank), CreateSquare(FILE_F, toRank));
			}
			else
			{
				// Queenside castle
				MovePiece(ColorToMove, PIECE_ROOK, CreateSquare(FILE_A, toRank), CreateSquare(FILE_D, toRank));
			}
		}
		else
		{
			MovePiece(ColorToMove, movingPiece, fromSquare, toSquare);
		}

		if (movingPiece == PIECE_KING)
		{
			Cache.KingSquare[ColorToMove] = toSquare;
			if (Colors[ColorToMove].CastleKingSide)
			{
				Hash.RemoveCastleKingside(ColorToMove);
				Colors[ColorToMove].CastleKingSide = false;
			}
			if (Colors[ColorToMove].CastleQueenSide)
			{
				Hash.RemoveCastleQueenside(ColorToMove);
				Colors[ColorToMove].CastleQueenSide = false;
			}
		}
		else if (movingPiece == PIECE_ROOK && fromRank == castleRank)
		{
			if (Colors[ColorToMove].CastleKingSide && fromFile == FILE_H)
			{
				Hash.RemoveCastleKingside(ColorToMove);
				Colors[ColorToMove].CastleKingSide = false;
			}
			else if (Colors[ColorToMove].CastleQueenSide && fromFile == FILE_A)
			{
				Hash.RemoveCastleQueenside(ColorToMove);
				Colors[ColorToMove].CastleQueenSide = false;
			}
		}

		Cache.CheckedBy = ZERO_BB;
		if (givesCheck)
			Cache.CheckedBy = GetAttackersTo(GetKingSquare(otherColor), ColorToMove, GetPieces());
		UpdateCheckInfo(ColorToMove);

		if (movingPiece == PIECE_PAWN || isCapture)
			HalfTurnsSinceCaptureOrPush = 0;
		else
			HalfTurnsSinceCaptureOrPush++;

		if (ColorToMove == COLOR_BLACK)
			TotalTurns++;

		ColorToMove = otherColor;
		Hash.FlipTeamToPlay();
	}

	bool Position::GivesCheck(Move move) const
	{
		STORM_ASSERT(ValidMove(move), "Invalid move");
		SquareIndex fromSquare = GetFromSquare(move);
		SquareIndex toSquare = GetToSquare(move);
		Piece movingPiece = TypeOf(GetPieceOnSquare(fromSquare));

		// Direct check
		if (Cache.CheckSquares[OtherColor(ColorToMove)][movingPiece - PIECE_START] & toSquare)
			return true;

		// Discovered check
		if (GetBlockersForKing(OtherColor(ColorToMove)) & fromSquare && !IsAligned(fromSquare, toSquare, GetKingSquare(OtherColor(ColorToMove))))
			return true;

		if (GetMoveType(move) == PROMOTION)
			return GetAttacksDynamic(GetPromotionPiece(move), toSquare, GetPieces() ^ fromSquare) & GetKingSquare(OtherColor(ColorToMove));

		if (movingPiece == PIECE_PAWN && toSquare == EnpassantSquare)
		{
			SquareIndex capturedSquare = GetEnpassantSquare(toSquare, ColorToMove);
			BitBoard blockers = (GetPieces() ^ fromSquare ^ capturedSquare) | toSquare;

			return ((GetAttacks<PIECE_BISHOP>(GetKingSquare(OtherColor(ColorToMove)), blockers) & GetPieces(ColorToMove, PIECE_BISHOP, PIECE_QUEEN)) |
					(GetAttacks<PIECE_ROOK>(GetKingSquare(OtherColor(ColorToMove)), blockers) & GetPieces(ColorToMove, PIECE_ROOK, PIECE_QUEEN)));
		}

		if (GetMoveType(move) == CASTLE)
		{
			File fromFile = FileOf(fromSquare);
			Rank fromRank = RankOf(fromSquare);
			File toFile = FileOf(toSquare);
			if (toFile == FILE_G)
			{
				// Kingside castle
				SquareIndex rookFrom = CreateSquare(FILE_H, fromRank);
				SquareIndex rookTo = CreateSquare(FILE_F, fromRank);
				return GetAttacks<PIECE_ROOK>(rookTo, (GetPieces() ^ rookFrom ^ fromSquare) | toSquare | rookTo) & GetKingSquare(OtherColor(ColorToMove));

			}
			else
			{
				// Queenside castle
				SquareIndex rookFrom = CreateSquare(FILE_A, fromRank);
				SquareIndex rookTo = CreateSquare(FILE_D, fromRank);
				return GetAttacks<PIECE_ROOK>(rookTo, (GetPieces() ^ rookFrom ^ fromSquare) | toSquare | rookTo) & GetKingSquare(OtherColor(ColorToMove));
			}
		}
		return false;
	}

	bool Position::IsLegal(Move move) const
	{
		SquareIndex kingSquare = GetKingSquare(ColorToMove);
		SquareIndex fromSquare = GetFromSquare(move);
		SquareIndex toSquare = GetToSquare(move);
		Piece movingPiece = TypeOf(GetPieceOnSquare(fromSquare));
		if (toSquare == EnpassantSquare && movingPiece == PIECE_PAWN)
		{
			SquareIndex capturedSquare = GetEnpassantSquare(toSquare, ColorToMove);
			BitBoard occupied = (GetPieces() ^ fromSquare ^ capturedSquare) | toSquare;
			return !(GetAttacks<PIECE_ROOK>(kingSquare, occupied) & GetPieces(OtherColor(ColorToMove), PIECE_QUEEN, PIECE_ROOK))
				&& !(GetAttacks<PIECE_BISHOP>(kingSquare, occupied) & GetPieces(OtherColor(ColorToMove), PIECE_QUEEN, PIECE_BISHOP));
		}

		if (Cache.CheckedBy)
		{
			if (GetMoveType(move) == CASTLE)
				return false;
			if (movingPiece != PIECE_KING)
			{
				if (MoreThanOne(Cache.CheckedBy))
					return false;
				SquareIndex checkingSquare = LeastSignificantBit(Cache.CheckedBy);
				if (((GetBitBoardBetween(checkingSquare, kingSquare) | Cache.CheckedBy) & toSquare) == ZERO_BB)
					return false;
			}
			else if (GetAttackersTo(toSquare, OtherColor(ColorToMove), GetPieces() ^ fromSquare))
			{
				return false;
			}
		}

		if (GetMoveType(move) == CASTLE)
		{
			Rank rank = RankOf(fromSquare);
			File file = FileOf(toSquare);
			if (file == FILE_C)
			{
				for (SquareIndex square : { CreateSquare(FILE_C, rank), CreateSquare(FILE_D, rank) })
				{
					if (GetAttackersTo(square, OtherColor(ColorToMove), GetPieces()))
						return false;
				}
			}
			else
			{
				for (SquareIndex square : { CreateSquare(FILE_F, rank), CreateSquare(FILE_G, rank) })
				{
					if (GetAttackersTo(square, OtherColor(ColorToMove), GetPieces()))
						return false;
				}
			}
			return true;
		}

		if (movingPiece == PIECE_KING)
		{
			return !GetAttackersTo(toSquare, OtherColor(ColorToMove), GetPieces() ^ fromSquare);
		}

		return !(GetBlockersForKing(ColorToMove) & fromSquare)
			  || IsAligned(fromSquare, toSquare, kingSquare);
	}

	BitBoard Position::GetSliderBlockers(BitBoard sliders, SquareIndex toSquare, BitBoard* pinners) const
	{
		BitBoard blockers = ZERO_BB;
		*pinners = ZERO_BB;
		BitBoard snipers = ((GetAttacks<PIECE_ROOK>(toSquare, ZERO_BB) & GetPieces(PIECE_QUEEN, PIECE_ROOK)) | (GetAttacks<PIECE_BISHOP>(toSquare, ZERO_BB) & GetPieces(PIECE_QUEEN, PIECE_BISHOP))) & sliders;
		BitBoard occupied = GetPieces() ^ snipers;
		Color color = GetColorOnSquare(toSquare);
		while (snipers)
		{
			SquareIndex square = PopLeastSignificantBit(snipers);
			BitBoard between = GetBitBoardBetween(square, toSquare) & occupied;
			if (between != ZERO_BB && !MoreThanOne(between))
			{
				blockers |= between;
				if (between & GetPieces(color))
					(*pinners) |= square;
			}
		}
		return blockers;
	}

	BitBoard Position::GetAttackersTo(SquareIndex square, Color by, BitBoard blockers) const
	{
		return (GetAttacks<PIECE_PAWN>(square, OtherColor(by)) & GetPieces(by, PIECE_PAWN)) |
			(GetAttacks<PIECE_KNIGHT>(square) & GetPieces(by, PIECE_KNIGHT)) |
			(GetAttacks<PIECE_KING>(square) & GetPieces(by, PIECE_KING)) |
			(GetAttacks<PIECE_BISHOP>(square, blockers) & GetPieces(by, PIECE_BISHOP, PIECE_QUEEN)) |
			(GetAttacks<PIECE_ROOK>(square, blockers) & GetPieces(by, PIECE_ROOK, PIECE_QUEEN));
	}

	BitBoard Position::GetAttackersTo(SquareIndex square, BitBoard blockers) const
	{
		// Squares that pawns attack FROM are found by usng the pawn attacks of the OPPOSITE color
		return (GetAttacks<PIECE_PAWN>(square, COLOR_BLACK) & GetPieces(COLOR_WHITE, PIECE_PAWN)) |
			(GetAttacks<PIECE_PAWN>(square, COLOR_WHITE) & GetPieces(COLOR_BLACK, PIECE_PAWN)) |
			(GetAttacks<PIECE_KNIGHT>(square) & GetPieces(PIECE_KNIGHT)) |
			(GetAttacks<PIECE_KING>(square) & GetPieces(PIECE_KING)) |
			(GetAttacks<PIECE_BISHOP>(square, blockers) & GetPieces(PIECE_BISHOP, PIECE_QUEEN)) |
			(GetAttacks<PIECE_ROOK>(square, blockers) & GetPieces(PIECE_ROOK, PIECE_QUEEN));
	}

	bool Position::SeeGE(Move move, ValueType threshold) const
	{
		if (GetMoveType(move) != NORMAL)
			return VALUE_DRAW >= threshold;
		SquareIndex from = GetFromSquare(move);
		SquareIndex to = GetToSquare(move);
		Piece captured = TypeOf(GetPieceOnSquare(to));
		BitBoard stmAttackers;

		ValueType swap = GetPieceValueMg(captured) - threshold;
		if (swap < 0)
			return false;

		ColorPiece movingPiece = GetPieceOnSquare(from);

		// Assume they capture piece for free
		swap = GetPieceValueMg(TypeOf(movingPiece)) - swap;
		if (swap <= 0)
			return true;

		Color stm = ColorOf(movingPiece);
		BitBoard occupied = GetPieces() ^ from ^ to;
		BitBoard attackers = GetAttackersTo(to, occupied);
		BitBoard bb;

		int res = 1;

		while (true)
		{
			stm = OtherColor(stm);
			attackers &= occupied;

			stmAttackers = attackers & GetPieces(stm);
			if (stmAttackers == ZERO_BB)
				break;

			if ((Cache.Pinners[OtherColor(stm)] & occupied) != ZERO_BB)
				stmAttackers &= ~GetBlockersForKing(stm);

			if (!stmAttackers)
				break;

			res ^= 1;

			if ((bb = stmAttackers & GetPieces(PIECE_PAWN)))
			{
				if ((swap = PawnValueMg - swap) < res)
					break;

				occupied ^= LeastSignificantBit(bb);
				attackers |= GetAttacks<PIECE_BISHOP>(to, occupied) & GetPieces(PIECE_BISHOP, PIECE_QUEEN);
			}

			else if ((bb = stmAttackers & GetPieces(PIECE_KNIGHT)))
			{
				if ((swap = KnightValueMg - swap) < res)
					break;

				occupied ^= LeastSignificantBit(bb);
			}

			else if ((bb = stmAttackers & GetPieces(PIECE_BISHOP)))
			{
				if ((swap = BishopValueMg - swap) < res)
					break;

				occupied ^= LeastSignificantBit(bb);
				attackers |= GetAttacks<PIECE_BISHOP>(to, occupied) & GetPieces(PIECE_BISHOP, PIECE_QUEEN);
			}

			else if ((bb = stmAttackers & GetPieces(PIECE_ROOK)))
			{
				if ((swap = RookValueMg - swap) < res)
					break;

				occupied ^= LeastSignificantBit(bb);
				attackers |= GetAttacks<PIECE_ROOK>(to, occupied) & GetPieces(PIECE_ROOK, PIECE_QUEEN);
			}

			else if ((bb = stmAttackers & GetPieces(PIECE_QUEEN)))
			{
				if ((swap = QueenValueMg - swap) < res)
					break;

				occupied ^= LeastSignificantBit(bb);
				attackers |= (GetAttacks<PIECE_BISHOP>(to, occupied) & GetPieces(PIECE_BISHOP, PIECE_QUEEN))
					| (GetAttacks<PIECE_ROOK>(to, occupied) & GetPieces(PIECE_ROOK, PIECE_QUEEN));
			}

			else // KING
					// If we "capture" with the king but opponent still has attackers,
					// reverse the result.
				return (attackers & ~GetPieces(stm)) ? res ^ 1 : res;
		}
		return bool(res);
	}

	void Position::MovePiece(Color color, Piece piece, SquareIndex from, SquareIndex to)
	{
		const BitBoard mask = from | to;
		Colors[color].Pieces[piece] ^= mask;
		Cache.ColorPieces[color] ^= mask;
		Cache.AllPieces ^= mask;
		Cache.PiecesByType[piece] ^= mask;
		Cache.PieceOnSquare[from] = COLOR_PIECE_NONE;
		Cache.PieceOnSquare[to] = CreatePiece(piece, color);

		Hash.RemovePieceAt(color, piece, from);
		Hash.AddPieceAt(color, piece, to);
	}

	void Position::AddPiece(Color color, Piece piece, SquareIndex square)
	{
		STORM_ASSERT(piece != PIECE_KING, "Cannot add king");
		Colors[color].Pieces[piece] |= square;
		Cache.ColorPieces[color] |= square;
		Cache.AllPieces |= square;
		Cache.PiecesByType[piece] |= square;
		Cache.PieceOnSquare[square] = CreatePiece(piece, color);
		if (piece != PIECE_PAWN)
		{
			Cache.NonPawnMaterial[color] += GetPieceValueMg(piece);
		}
		Hash.AddPieceAt(color, piece, square);
	}

	void Position::RemovePiece(Color color, Piece piece, SquareIndex square)
	{
		STORM_ASSERT(piece != PIECE_KING, "Cannot remove king");
		Colors[color].Pieces[piece] ^= square;
		Cache.ColorPieces[color] ^= square;
		Cache.AllPieces ^= square;
		Cache.PiecesByType[piece] ^= square;
		Cache.PieceOnSquare[square] = COLOR_PIECE_NONE;
		if (piece != PIECE_PAWN)
		{
			Cache.NonPawnMaterial[color] -= GetPieceValueEg(piece);
		}
		Hash.RemovePieceAt(color, piece, square);
	}

	void Position::UpdateCheckInfo(Color color)
	{
		Cache.BlockersForKing[COLOR_WHITE] = GetSliderBlockers(GetPieces(COLOR_BLACK), GetKingSquare(COLOR_WHITE), &Cache.Pinners[COLOR_BLACK]);
		Cache.BlockersForKing[COLOR_BLACK] = GetSliderBlockers(GetPieces(COLOR_WHITE), GetKingSquare(COLOR_BLACK), &Cache.Pinners[COLOR_WHITE]);

		SquareIndex kingSquare = GetKingSquare(color);

		Cache.CheckSquares[color][PIECE_PAWN - PIECE_START] = GetAttacks<PIECE_PAWN>(kingSquare, color);
		Cache.CheckSquares[color][PIECE_KNIGHT - PIECE_START] = GetAttacks<PIECE_KNIGHT>(kingSquare);
		Cache.CheckSquares[color][PIECE_BISHOP - PIECE_START] = GetAttacks<PIECE_BISHOP>(kingSquare, GetPieces());
		Cache.CheckSquares[color][PIECE_ROOK - PIECE_START] = GetAttacks<PIECE_ROOK>(kingSquare, GetPieces());
		Cache.CheckSquares[color][PIECE_QUEEN - PIECE_START] = Cache.CheckSquares[color][PIECE_BISHOP - PIECE_START] | Cache.CheckSquares[color][PIECE_ROOK - PIECE_START];
		Cache.CheckSquares[color][PIECE_KING - PIECE_START] = ZERO_BB;
	}

    void ClearPosition(Position& position)
    {
        for (Piece piece = PIECE_START; piece < PIECE_MAX; piece++)
        {
            position.Colors[COLOR_WHITE].Pieces[piece] = ZERO_BB;
            position.Colors[COLOR_BLACK].Pieces[piece] = ZERO_BB;
        }
        position.EnpassantSquare = SQUARE_INVALID;
        position.ColorToMove = COLOR_WHITE;
        position.TotalTurns = 0;
        position.HalfTurnsSinceCaptureOrPush = 0;
    }

    Position CreateStartingPosition()
    {
        Position position;
        ClearPosition(position);

        position.Colors[COLOR_WHITE].Pieces[PIECE_PAWN] |= a2;
        position.Colors[COLOR_WHITE].Pieces[PIECE_PAWN] |= b2;
        position.Colors[COLOR_WHITE].Pieces[PIECE_PAWN] |= c2;
        position.Colors[COLOR_WHITE].Pieces[PIECE_PAWN] |= d2;
        position.Colors[COLOR_WHITE].Pieces[PIECE_PAWN] |= e2;
        position.Colors[COLOR_WHITE].Pieces[PIECE_PAWN] |= f2;
        position.Colors[COLOR_WHITE].Pieces[PIECE_PAWN] |= g2;
        position.Colors[COLOR_WHITE].Pieces[PIECE_PAWN] |= h2;

        position.Colors[COLOR_WHITE].Pieces[PIECE_KNIGHT] = b1 | g1;
        position.Colors[COLOR_WHITE].Pieces[PIECE_BISHOP] = c1 | f1;
        position.Colors[COLOR_WHITE].Pieces[PIECE_ROOK]   = a1 | h1;
        position.Colors[COLOR_WHITE].Pieces[PIECE_QUEEN]  = GetSquareBB(d1);
        position.Colors[COLOR_WHITE].Pieces[PIECE_KING]   = GetSquareBB(e1);

        position.Colors[COLOR_BLACK].Pieces[PIECE_PAWN] |= a7;
        position.Colors[COLOR_BLACK].Pieces[PIECE_PAWN] |= b7;
        position.Colors[COLOR_BLACK].Pieces[PIECE_PAWN] |= c7;
        position.Colors[COLOR_BLACK].Pieces[PIECE_PAWN] |= d7;
        position.Colors[COLOR_BLACK].Pieces[PIECE_PAWN] |= e7;
        position.Colors[COLOR_BLACK].Pieces[PIECE_PAWN] |= f7;
        position.Colors[COLOR_BLACK].Pieces[PIECE_PAWN] |= g7;
        position.Colors[COLOR_BLACK].Pieces[PIECE_PAWN] |= h7;

        position.Colors[COLOR_BLACK].Pieces[PIECE_KNIGHT] = b8 | g8;
        position.Colors[COLOR_BLACK].Pieces[PIECE_BISHOP] = c8 | f8;
        position.Colors[COLOR_BLACK].Pieces[PIECE_ROOK]   = a8 | h8;
        position.Colors[COLOR_BLACK].Pieces[PIECE_QUEEN]  = GetSquareBB(d8);
        position.Colors[COLOR_BLACK].Pieces[PIECE_KING]   = GetSquareBB(e8);

		position.Colors[COLOR_WHITE].CastleKingSide = true;
		position.Colors[COLOR_WHITE].CastleQueenSide = true;
		position.Colors[COLOR_BLACK].CastleKingSide = true;
		position.Colors[COLOR_BLACK].CastleQueenSide = true;

		position.EnpassantSquare = SQUARE_INVALID;

        position.Initialize();
        return position;
    }

    Position CreatePositionFromFEN(const std::string& fen)
    {
		Position position;
		ClearPosition(position);
		File currentFile = FILE_A;
		Rank currentRank = RANK_8;
		int index = 0;
		for (char c : fen)
		{
			index++;
			if (c == ' ')
				break;
			if (std::isdigit(c))
			{
				int count = c - '0';
				currentFile = (File)((int)currentFile + count);
			}
			if (c == '/')
			{
				currentRank--;
				currentFile = FILE_A;
			}
			switch (c)
			{
			case 'P':
				position.Colors[COLOR_WHITE].Pieces[PIECE_PAWN] |= CreateSquare(currentFile++, currentRank);
				break;
			case 'N':
				position.Colors[COLOR_WHITE].Pieces[PIECE_KNIGHT] |= CreateSquare(currentFile++, currentRank);
				break;
			case 'B':
				position.Colors[COLOR_WHITE].Pieces[PIECE_BISHOP] |= CreateSquare(currentFile++, currentRank);
				break;
			case 'R':
				position.Colors[COLOR_WHITE].Pieces[PIECE_ROOK] |= CreateSquare(currentFile++, currentRank);
				break;
			case 'Q':
				position.Colors[COLOR_WHITE].Pieces[PIECE_QUEEN] |= CreateSquare(currentFile++, currentRank);
				break;
			case 'K':
				position.Colors[COLOR_WHITE].Pieces[PIECE_KING] |= CreateSquare(currentFile++, currentRank);
				break;
			case 'p':
				position.Colors[COLOR_BLACK].Pieces[PIECE_PAWN] |= CreateSquare(currentFile++, currentRank);
				break;
			case 'n':
				position.Colors[COLOR_BLACK].Pieces[PIECE_KNIGHT] |= CreateSquare(currentFile++, currentRank);
				break;
			case 'b':
				position.Colors[COLOR_BLACK].Pieces[PIECE_BISHOP] |= CreateSquare(currentFile++, currentRank);
				break;
			case 'r':
				position.Colors[COLOR_BLACK].Pieces[PIECE_ROOK] |= CreateSquare(currentFile++, currentRank);
				break;
			case 'q':
				position.Colors[COLOR_BLACK].Pieces[PIECE_QUEEN] |= CreateSquare(currentFile++, currentRank);
				break;
			case 'k':
				position.Colors[COLOR_BLACK].Pieces[PIECE_KING] |= CreateSquare(currentFile++, currentRank);
				break;
			}
		}
		position.ColorToMove = (fen[index] == 'w') ? COLOR_WHITE : COLOR_BLACK;
		index += 2;

		position.Colors[COLOR_WHITE].CastleKingSide = false;
		position.Colors[COLOR_WHITE].CastleQueenSide = false;
		position.Colors[COLOR_BLACK].CastleKingSide = false;
		position.Colors[COLOR_BLACK].CastleQueenSide = false;

		while (fen[index] != ' ')
		{
			switch (fen[index])
			{
			case 'K':
				position.Colors[COLOR_WHITE].CastleKingSide = true;
				break;
			case 'Q':
				position.Colors[COLOR_WHITE].CastleQueenSide = true;
				break;
			case 'k':
				position.Colors[COLOR_BLACK].CastleKingSide = true;
				break;
			case 'q':
				position.Colors[COLOR_BLACK].CastleQueenSide = true;
				break;
			}
			index++;
		}
		index++;

		if (fen[index] != '-')
		{
			position.EnpassantSquare = UCI::SquareFromString(fen.substr(index, 2));
			index += 3;
		}
		else
		{
			index += 2;
		}

		size_t space = fen.find_first_of(' ', index);
		if (space != std::string::npos)
		{
			position.HalfTurnsSinceCaptureOrPush = std::stoi(fen.substr(index, space - index));
			index = int(space) + 1;
			space = fen.find_first_of(' ', index);
			position.TotalTurns = std::stoi(fen.substr(index)) - 1;
		}
		else
		{
			position.HalfTurnsSinceCaptureOrPush = 0;
			position.TotalTurns = 0;
		}

		position.Initialize();
		return position;
    }

    std::string GetFENFromPosition(const Position& position)
    {
		std::string result = "";

		for (Rank rank = RANK_8; rank >= RANK_1; rank--)
		{
			int emptyCount = 0;
			for (File file = FILE_A; file < FILE_MAX; file++)
			{
				char pieceFEN = 0;
				ColorPiece colorPiece = position.GetPieceOnSquare(CreateSquare(file, rank));
				Piece piece = TypeOf(colorPiece);
				Color c = ColorOf(colorPiece);
				if (piece != PIECE_NONE)
					pieceFEN = UCI::PieceToString(piece, c);
				if (pieceFEN != 0)
				{
					if (emptyCount > 0)
					{
						result += (char)(emptyCount + '0');
						emptyCount = 0;
					}
					result += pieceFEN;
				}
				else
				{
					emptyCount++;
				}
			}
			if (emptyCount > 0)
			{
				result += (char)(emptyCount + '0');
				emptyCount = 0;
			}
			if (rank != RANK_1)
				result += '/';
		}

		result += (position.ColorToMove == COLOR_WHITE) ? " w" : " b";
		result += ' ';
		if (position.Colors[COLOR_WHITE].CastleKingSide || position.Colors[COLOR_WHITE].CastleQueenSide || position.Colors[COLOR_BLACK].CastleKingSide || position.Colors[COLOR_BLACK].CastleQueenSide)
		{
			if (position.Colors[COLOR_WHITE].CastleKingSide)
				result += 'K';
			if (position.Colors[COLOR_WHITE].CastleQueenSide)
				result += 'Q';
			if (position.Colors[COLOR_BLACK].CastleKingSide)
				result += 'k';
			if (position.Colors[COLOR_BLACK].CastleQueenSide)
				result += 'q';
		}
		else
		{
			result += '-';
		}

		if (position.EnpassantSquare == SQUARE_INVALID)
			result += " -";
		else
			result += " " + UCI::SquareToString(position.EnpassantSquare);

		result += " " + std::to_string(position.HalfTurnsSinceCaptureOrPush);
		result += " " + std::to_string(position.TotalTurns + 1);

		return result;
    }

	std::string FormatPosition(const Position& position)
	{
		std::stringstream stream;
		for (Rank rank = RANK_8; rank >= RANK_1; rank--)
		{
			stream << "   +";
			for (File file = FILE_A; file < FILE_MAX; file++)
			{
				stream << "---+";
			}
			stream << "\n " << char('1' + (rank - RANK_1)) << " |";
			for (File file = FILE_A; file < FILE_MAX; file++)
			{
				char pieceFen = ' ';
				SquareIndex square = CreateSquare(file, rank);
				ColorPiece piece = position.GetPieceOnSquare(square);
				if (piece != COLOR_PIECE_NONE)
				{
					pieceFen = UCI::PieceToString(TypeOf(piece), ColorOf(piece));
				}
				stream << ' ' << pieceFen << " |";
			}
			stream << '\n';
		}
		stream << "   +";
		for (File file = FILE_A; file < FILE_MAX; file++)
		{
			stream << "---+";
		}
		stream << "\n    ";
		for (File file = FILE_A; file < FILE_MAX; file++)
		{
			stream << ' ' << char('A' + (file - FILE_A)) << "  ";
		}
		return stream.str();
	}

	std::ostream& operator<<(std::ostream& stream, const Position& position)
	{
		stream << FormatPosition(position);
		return stream;
	}

}
