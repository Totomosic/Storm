#include "Format.h"

namespace Storm
{
    char UCI::PieceToString(Piece type, Color color)
    {
        char offset = color == COLOR_BLACK ? 'a' - 'A' : 0;
        switch (type)
        {
        case PIECE_PAWN:
            return 'P' + offset;
        case PIECE_KNIGHT:
            return 'N' + offset;
        case PIECE_BISHOP:
            return 'B' + offset;
        case PIECE_ROOK:
            return 'R' + offset;
        case PIECE_QUEEN:
            return 'Q' + offset;
        case PIECE_KING:
            return 'K' + offset;
        default:
            STORM_ASSERT(false, "Invalid PIECE");
            break;
        }
        return 'P' + offset;
    }
    SquareIndex UCI::SquareFromString(const std::string& square)
    {
        STORM_ASSERT(square.size() == 2, "Invalid square string");
        char file = square[0];
        char rank = square[1];
        return CreateSquare(File(file - 'a'), Rank(rank - '1'));
    }

    std::string UCI::SquareToString(SquareIndex square)
    {
        File file = FileOf(square);
        Rank rank = RankOf(square);
        return std::string({ char(file + 'a'), char(rank + '1') });
    }

    std::string UCI::FormatMove(Move move)
    {
        SquareIndex from = GetFromSquare(move);
        SquareIndex to = GetToSquare(move);
        return SquareToString(from) + SquareToString(to);
    }

}
