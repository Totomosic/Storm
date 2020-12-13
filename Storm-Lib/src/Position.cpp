#include "Position.h"
#include "Format.h"
#include "EvalConstants.h"

namespace Storm
{

	void Position::ApplyMove(Move move)
	{
		STORM_ASSERT(move != MOVE_NONE, "Invalid move");
		if (EnpassantSquare != SQUARE_INVALID)
		{
			Hash.RemoveEnPassant(FileOf(EnpassantSquare));
			EnpassantSquare = SQUARE_INVALID;
		}
		const SquareIndex fromSquare = GetFromSquare(move);
		const SquareIndex toSquare = GetToSquare(move);
		const Rank fromRank = RankOf(fromSquare);
		const Rank toRank = RankOf(toSquare);
		const File fromFile = FileOf(fromSquare);
		const File toFile = FileOf(toSquare);
		const Piece movingPiece = GetMovingPiece(move);
		const Piece capturedPiece = GetCapturedPiece(move);
		const bool isPromotion = movingPiece == PIECE_PAWN && toRank == GetPromotionRank(ColorToMove);
		const bool isCapture = capturedPiece != PIECE_NONE;
		const bool isCastle = movingPiece == PIECE_KING && fromFile == FILE_E && (toFile == FILE_C || toFile == FILE_G);
		const Color otherColor = OtherColor(ColorToMove);

		if (isCapture && isPromotion)
		{
			const Piece promotionPiece = GetPromotionPiece(move);
			RemovePiece(otherColor, capturedPiece, toSquare);
			RemovePiece(ColorToMove, movingPiece, fromSquare);
			AddPiece(ColorToMove, promotionPiece, toSquare);
			if (capturedPiece == PIECE_ROOK)
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
			if (capturedPiece == PIECE_ROOK)
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
		else if (movingPiece == PIECE_PAWN && toSquare == EnpassantSquare)
		{
			RemovePiece(otherColor, PIECE_PAWN, GetEnpassantSquare(toSquare, otherColor));
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
		else if (movingPiece == PIECE_ROOK)
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

		if (movingPiece == PIECE_PAWN || isCapture)
			HalfTurnsSinceCaptureOrPush = 0;
		else
			HalfTurnsSinceCaptureOrPush++;

		if (ColorToMove == COLOR_BLACK)
			TotalTurns++;

		ColorToMove = otherColor;
		Hash.FlipTeamToPlay();
	}

	void Position::MovePiece(Color color, Piece piece, SquareIndex from, SquareIndex to)
	{
		const BitBoard mask = from | to;
		Colors[color].Pieces[piece] ^= mask;
		Cache.ColorPieces[color] ^= mask;
		Cache.AllPieces ^= mask;
		Cache.PiecesByType[piece] ^= mask;
		Cache.PieceOnSquare[from] = PIECE_NONE;
		Cache.PieceOnSquare[to] = piece;

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
		Cache.PieceOnSquare[square] = piece;
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
		Cache.PieceOnSquare[square] = PIECE_NONE;
		if (piece != PIECE_PAWN)
		{
			Cache.NonPawnMaterial[color] -= GetPieceValueEg(piece);
		}
		Hash.RemovePieceAt(color, piece, square);
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

    void InitializePosition(Position& position)
    {
        position.Cache.NonPawnMaterial[COLOR_WHITE] = 0;
        position.Cache.NonPawnMaterial[COLOR_BLACK] = 0;

        for (SquareIndex sq = a1; sq < SQUARE_MAX; sq++)
            position.Cache.PieceOnSquare[sq] = PIECE_NONE;

		position.Cache.ColorPieces[COLOR_WHITE] = ZERO_BB;
		position.Cache.ColorPieces[COLOR_BLACK] = ZERO_BB;

		for (Piece piece = PIECE_START; piece < PIECE_MAX; piece++)
		{
			position.Cache.ColorPieces[COLOR_WHITE] |= position.Colors[COLOR_WHITE].Pieces[piece];
			position.Cache.ColorPieces[COLOR_BLACK] |= position.Colors[COLOR_BLACK].Pieces[piece];
		}

		position.Cache.AllPieces = position.GetPieces(COLOR_WHITE) | position.GetPieces(COLOR_BLACK);

        for (Piece piece = PIECE_START; piece < PIECE_MAX; piece++)
        {
            position.Cache.PiecesByType[piece] = position.GetPieces(piece);
            if (piece != PIECE_PAWN && piece != PIECE_KING)
            {
                position.Cache.NonPawnMaterial[COLOR_WHITE] += GetPieceValueMg(piece);
                position.Cache.NonPawnMaterial[COLOR_BLACK] += GetPieceValueMg(piece);
            }
            BitBoard pieces = position.GetPieces(piece);
            while (pieces)
            {
                SquareIndex square = PopLeastSignificantBit(pieces);
                position.Cache.PieceOnSquare[square] = piece;
            }
        }

        position.Cache.KingSquare[COLOR_WHITE] = LeastSignificantBit(position.GetPieces(COLOR_WHITE, PIECE_KING));
        position.Cache.KingSquare[COLOR_BLACK] = MostSignificantBit(position.GetPieces(COLOR_BLACK, PIECE_KING));
        STORM_ASSERT(position.Cache.KingSquare[COLOR_WHITE] != ZERO_BB && position.Cache.KingSquare[COLOR_BLACK] != ZERO_BB, "Couldn't find king");

        position.Hash.SetFromPosition(position);
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

        InitializePosition(position);
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

		InitializePosition(position);
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
				Piece piece = position.GetPieceOnSquare(CreateSquare(file, rank));
				Color c = position.GetColorAt(CreateSquare(file, rank));
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
				Piece piece = position.GetPieceOnSquare(square);
				if (piece != PIECE_NONE)
				{
					Color color = position.GetColorAt(square);
					pieceFen = UCI::PieceToString(piece, color);
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
