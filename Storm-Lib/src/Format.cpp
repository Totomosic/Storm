#include "Format.h"

namespace Storm
{

    std::vector<std::string> Split(const std::string& str, const std::string& delimiter)
    {
        std::vector<std::string> result;
        size_t begin = 0;
        size_t end = str.find(delimiter, begin);
        while (end != std::string::npos)
        {
            result.push_back(str.substr(begin, end - begin));
            begin = end + delimiter.size();
            end = str.find(delimiter, begin);
        }
        result.push_back(str.substr(begin, end - begin));
        return result;
    }

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

    Move UCI::CreateMoveFromString(const Position& position, const std::string& uciString)
    {
        STORM_ASSERT(uciString.size() >= 4, "Invalid move");
        File startFile = File(uciString[0] - 'a');
        Rank startRank = Rank(uciString[1] - '1');
        File endFile = File(uciString[2] - 'a');
        Rank endRank = Rank(uciString[3] - '1');
        Piece promotion = PIECE_QUEEN;
        if (uciString.size() >= 5)
        {
            // support lower case or upper case
            char promotionChar = uciString[4];
            if (promotionChar - 'a' < 0)
                promotionChar += 'a' - 'A';
            switch (promotionChar)
            {
            case 'q':
                promotion = PIECE_QUEEN;
                break;
            case 'n':
                promotion = PIECE_KNIGHT;
                break;
            case 'r':
                promotion = PIECE_ROOK;
                break;
            case 'b':
                promotion = PIECE_BISHOP;
                break;
            default:
                STORM_ASSERT(false, "Invalid promotion type: {}", promotionChar);
                break;
            }
        }
        if (position.GetPieceOnSquare(CreateSquare(startFile, startRank)) == PIECE_KING && startFile == FILE_E && (endFile == FILE_C || endFile == FILE_G))
        {
            return CreateMove(CreateSquare(startFile, startRank), CreateSquare(endFile, endRank), CASTLE);
        }
        return CreateMove(CreateSquare(startFile, startRank), CreateSquare(endFile, endRank));
    }

}
