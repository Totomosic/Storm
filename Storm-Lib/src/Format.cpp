#include "Format.h"
#include "Evaluation.h"

#include <fstream>

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

	void CleanupString(std::string& str, const char* charsToRemove, int charCount)
	{
		while (true)
		{
			bool found = false;
			for (int i = 0; i < charCount; i++)
			{
				size_t pos = str.find(charsToRemove[i]);
				if (pos != std::string::npos)
				{
					found = true;
					str.erase(str.begin() + pos);
				}
			}
			if (!found)
				break;
		}
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
        if (move == MOVE_NONE)
            return "(none)";
        SquareIndex from = GetFromSquare(move);
        SquareIndex to = GetToSquare(move);
        Piece promotion = GetPromotionPiece(move);
        std::string result = SquareToString(from) + SquareToString(to);
        if (promotion != PIECE_NONE && GetMoveType(move) == PROMOTION)
            result += PieceToString(promotion, COLOR_BLACK);
        return result;
    }

    Move UCI::CreateMoveFromString(const Position& position, const std::string& uciString)
    {
        STORM_ASSERT(uciString.size() >= 4, "Invalid move");
        File startFile = File(uciString[0] - 'a');
        Rank startRank = Rank(uciString[1] - '1');
        File endFile = File(uciString[2] - 'a');
        Rank endRank = Rank(uciString[3] - '1');
        Piece promotion = PIECE_NONE;
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
        if (TypeOf(position.GetPieceOnSquare(CreateSquare(startFile, startRank))) == PIECE_KING && startFile == FILE_E && (endFile == FILE_C || endFile == FILE_G))
            return CreateMove(CreateSquare(startFile, startRank), CreateSquare(endFile, endRank), CASTLE);
        if (promotion != PIECE_NONE && endRank == GetPromotionRank(position.ColorToMove))
            return CreateMove(CreateSquare(startFile, startRank), CreateSquare(endFile, endRank), promotion);
        return CreateMove(CreateSquare(startFile, startRank), CreateSquare(endFile, endRank));
    }


	std::string PGN::FormatMove(Move move, const Position& position)
	{
		std::string moveString;
		if (GetMoveType(move) == CASTLE)
		{
			if (FileOf(GetToSquare(move)) == FILE_G)
				moveString = "O-O";
			else
				moveString = "O-O-O";
		}
		else
		{
			SquareIndex from = GetFromSquare(move);
			SquareIndex to = GetToSquare(move);
			Piece movingPiece = TypeOf(position.GetPieceOnSquare(GetFromSquare(move)));
			moveString = GetPieceName(movingPiece);
			BitBoard pieceAttacks;
			if (movingPiece == PIECE_PAWN)
				pieceAttacks = GetAttacks<PIECE_PAWN>(to, OtherColor(position.ColorToMove));
			else
				pieceAttacks = GetAttacksDynamic(movingPiece, to, position.GetPieces());
			BitBoard attacks = pieceAttacks & position.GetPieces(position.ColorToMove, movingPiece);
			BitBoard pinnedAttacks = attacks & position.GetBlockersForKing(position.ColorToMove);
			while (pinnedAttacks)
			{
				SquareIndex square = PopLeastSignificantBit(pinnedAttacks);
				if (!IsAligned(square, position.GetKingSquare(position.ColorToMove), to))
				{
					attacks &= ~square;
				}
			}
			STORM_ASSERT(attacks != ZERO_BB || movingPiece == PIECE_PAWN, "Invalid move");
			if (MoreThanOne(attacks) && movingPiece != PIECE_PAWN)
			{
				// Resolve ambiguity
				if (!MoreThanOne(attacks & FILE_MASKS[FileOf(from)]))
					moveString += GetFileName(FileOf(from));
				else if (!MoreThanOne(attacks & RANK_MASKS[RankOf(from)]))
					moveString += GetRankName(RankOf(from));
				else
					moveString += UCI::SquareToString(from);
			}
			if (position.IsCapture(move))
			{
				if (movingPiece == PIECE_PAWN)
				{
					moveString += GetFileName(FileOf(from));
				}
				moveString += 'x';
			}
			moveString += UCI::SquareToString(to);
			if (GetMoveType(move) == PROMOTION)
			{
				moveString += "=" + GetPieceName(GetPromotionPiece(move));
			}
		}
		UndoInfo undo;
		Position pos = position;
		pos.ApplyMove(move, &undo);
		if (pos.InCheck())
		{
			Move buffer[MAX_MOVES];
			MoveList moves(buffer);
			moves.FillLegal<ALL>(pos);
			moveString += moves.Size() == 0 ? '#' : '+';
		}
		return moveString;
	}

	Move PGN::CreateMoveFromString(const Position& position, const std::string& pgnString)
	{
		Move move = MOVE_NONE;
		if (pgnString.substr(0, 5) == "O-O-O" || pgnString.substr(0, 5) == "0-0-0")
		{
			if (position.ColorToMove == COLOR_WHITE)
				return CreateMove(e1, c1, CASTLE);
			else
				return CreateMove(e8, c8, CASTLE);
		}
		if (pgnString.substr(0, 3) == "O-O" || pgnString.substr(0, 3) == "0-0")
		{
			if (position.ColorToMove == COLOR_WHITE)
				return CreateMove(e1, g1, CASTLE);
			else
				return CreateMove(e8, g8, CASTLE);
		}
		size_t equals = pgnString.find('=');
		Piece promotion = PIECE_NONE;
		if (equals != std::string::npos)
		{
			promotion = GetPieceFromName(pgnString[equals + 1]);
		}
		size_t endIndex = pgnString.size() - 1;
		if (pgnString[endIndex] == '#' || pgnString[endIndex] == '+')
			endIndex--;
		if (equals != std::string::npos)
			endIndex -= 2;
		SquareIndex toSquare = UCI::SquareFromString(pgnString.substr(endIndex - 1, 2));
		Piece movingPiece = PIECE_PAWN;
		File pawnFile = FILE_MAX;

		bool isCapture = pgnString.find('x') != std::string::npos;

		if (IsCapital(pgnString[0]))
			movingPiece = GetPieceFromName(pgnString[0]);
		else
		{
			pawnFile = GetFileFromName(pgnString[0]);
		}

		if (movingPiece == PIECE_PAWN && !isCapture)
		{
			Rank toRank = RankOf(toSquare);
			if (position.ColorToMove == COLOR_WHITE)
			{
				BitBoard pawns = position.GetPieces(position.ColorToMove, PIECE_PAWN) & FILE_MASKS[pawnFile] & ~InFrontOrEqual<COLOR_WHITE>(toRank);
				move = CreateMove(FrontmostSquare<COLOR_WHITE>(pawns), toSquare);
				if (promotion != PIECE_NONE)
					move = CreateMove(FrontmostSquare<COLOR_WHITE>(pawns), toSquare, promotion);
			}
			else
			{
				BitBoard pawns = position.GetPieces(position.ColorToMove, PIECE_PAWN) & FILE_MASKS[pawnFile] & ~InFrontOrEqual<COLOR_BLACK>(toRank);
				move = CreateMove(FrontmostSquare<COLOR_BLACK>(pawns), toSquare);
				if (promotion != PIECE_NONE)
					move = CreateMove(FrontmostSquare<COLOR_BLACK>(pawns), toSquare, promotion);
			}
		}
		else
		{
			BitBoard attacks;
			if (movingPiece == PIECE_PAWN)
				attacks = GetAttacks<PIECE_PAWN>(toSquare, OtherColor(position.ColorToMove));
			else
				attacks = GetAttacksDynamic(movingPiece, toSquare, position.GetPieces());
			attacks &= position.GetPieces(position.ColorToMove, movingPiece);
			BitBoard pinnedAttacks = attacks & position.GetBlockersForKing(position.ColorToMove);
			while (pinnedAttacks)
			{
				SquareIndex square = PopLeastSignificantBit(pinnedAttacks);
				if (!IsAligned(square, position.GetKingSquare(position.ColorToMove), toSquare))
				{
					attacks &= ~square;
				}
			}
			if (movingPiece == PIECE_PAWN && pawnFile != FILE_MAX)
				attacks &= FILE_MASKS[pawnFile];
			if (attacks == ZERO_BB)
				return MOVE_NONE;
			if (MoreThanOne(attacks))
			{
				SquareIndex fromSquare;
				char disambiguation = pgnString[1];
				if (std::isdigit(disambiguation))
				{
					// Rank
					fromSquare = MostSignificantBit(attacks & RANK_MASKS[GetRankFromName(disambiguation)]);
				}
				else
				{
					// File/Square
					attacks = attacks & FILE_MASKS[GetFileFromName(disambiguation)];
					if (MoreThanOne(attacks))
					{
						attacks = attacks & RANK_MASKS[GetRankFromName(pgnString[2])];
					}
					fromSquare = MostSignificantBit(attacks);
				}
				move = CreateMove(fromSquare, toSquare);
				if (promotion != PIECE_NONE)
					move = CreateMove(fromSquare, toSquare, promotion);
			}
			else
			{
				SquareIndex from = MostSignificantBit(attacks);
				move = CreateMove(from, toSquare);
				if (promotion != PIECE_NONE)
					move = CreateMove(from, toSquare, promotion);
			}
		}
		if (position.IsCapture(move) == isCapture && ((promotion != PIECE_NONE) == (GetMoveType(move) == PROMOTION)))
			return move;
		STORM_ASSERT(false, "Invalid PGN Move");
		return MOVE_NONE;
	}

	std::vector<PGNMatch> PGN::ReadFromString(const std::string& pgn)
	{
		enum ReadStage
		{
			TAGS,
			MOVES,
		};

		constexpr char CharsToRemove[] = { '\r' };
		constexpr char TagCharsToRemove[] = { '\\' };
		std::vector<PGNMatch> matches;
		std::vector<std::string> lines = Split(pgn, "\n");

		ReadStage currentStage = TAGS;
		PGNMatch currentMatch;
		Position currentPosition = CreateStartingPosition();
		currentMatch.InitialPosition = currentPosition;
		bool inComment = false;
		std::string result = "";

		for (std::string& line : lines)
		{
			CleanupString(line, CharsToRemove, sizeof(CharsToRemove) / sizeof(char));
			if (line.size() > 0)
			{
				if (line[0] == '[')
				{
					if (currentStage != TAGS)
					{
						// Start new Match
						matches.push_back(currentMatch);
						currentMatch = PGNMatch();
						currentStage = TAGS;
						currentPosition = CreateStartingPosition();
						inComment = false;
						currentMatch.InitialPosition = currentPosition;
						result = "";
					}
					size_t firstSpace = line.find_first_of(' ');
					if (firstSpace != std::string::npos)
					{
						std::string tagName = line.substr(1, firstSpace - 1);
						size_t firstQuote = line.find_first_of('"', firstSpace);
						size_t lastQuote = line.find_last_of('"');
						if (firstQuote != std::string::npos && firstQuote != lastQuote)
						{
							std::string tagValue = line.substr(firstQuote + 1, lastQuote - firstQuote - 1);
							CleanupString(tagValue, TagCharsToRemove, sizeof(TagCharsToRemove) / sizeof(char));
							currentMatch.Tags[tagName] = tagValue;

							if (tagName == "FEN")
							{
								currentPosition = CreatePositionFromFEN(tagValue);
								currentMatch.InitialPosition = currentPosition;
							}
							else if (tagName == "Result")
								result = tagValue;
							else if (tagName == "Variant")
								std::cout << tagValue << std::endl;
						}
						else
						{
							std::cout << "Invalid Tag Line: Missing start or end quote" << std::endl;
						}
					}
					else
					{
						std::cout << "Invalid Tag Line: Missing Space" << std::endl;
					}
				}
				if (currentStage == TAGS && std::isdigit(line[0]))
				{
					currentStage = MOVES;
				}
				if (currentStage == MOVES)
				{
					size_t current = 0;
					if (inComment)
					{
						current = line.find_first_of('}');
						if (current != std::string::npos)
							current++;
						else
							continue;
					}
					size_t end = line.find_first_of(' ', current);
					while (current != std::string::npos && current < line.size())
					{
						std::string str;
						if (end != std::string::npos)
							str = line.substr(current, end - current);
						else
							str = line.substr(current);
						if (str.size() > 0)
						{
							if (str[0] == '{')
							{
								inComment = true;
								end = line.find_first_of('}', current);
								if (end != std::string::npos)
									inComment = false;
							}
							else if (str[0] == ';' || str == result)
								break;
							else
							{
								bool hasPeriod = str.find('.') != std::string::npos;
								bool valid = !hasPeriod;
								if (hasPeriod && str.find('.') != str.size() - 1)
								{
									valid = true;
									str = str.substr(str.find('.') + 1);
								}
								if (valid)
								{
									Move buffer[MAX_MOVES];
									MoveList legalMoves(buffer);
									legalMoves.FillLegal<ALL>(currentPosition);
									Move move = CreateMoveFromString(currentPosition, str);
									if (move != MOVE_NONE && std::find(legalMoves.begin(), legalMoves.end(), move) != legalMoves.end())
									{
										STORM_ASSERT(FormatMove(move, currentPosition) == str, "Invalid move");
										currentMatch.Moves.push_back(move);
										UndoInfo undo;
										currentPosition.ApplyMove(move, &undo);
									}
									else
									{
										if (move == MOVE_NONE)
										{
											std::cout << currentPosition << std::endl;
											std::cout << "Side to Move: " << ((currentPosition.ColorToMove == COLOR_WHITE) ? 'w' : 'b') << std::endl;
											std::cout << "Invalid move: " << str << std::endl;
										}
										else
										{
											std::cout << currentPosition << std::endl;
											std::cout << "Side to Move: " << ((currentPosition.ColorToMove == COLOR_WHITE) ? 'w' : 'b') << std::endl;
											std::cout << "Illegal move: " << str << std::endl;
											std::cout << "Legal Moves: " << std::endl;
											for (Move move : legalMoves)
												std::cout << FormatMove(move, currentPosition) << std::endl;
										}
									}
								}
							}
						}
						if (end != std::string::npos)
						{
							current = end + 1;
							end = line.find_first_of(' ', current);
						}
						else
						{
							break;
						}
					}
				}
			}
		}
		matches.push_back(currentMatch);

		return matches;
	}

	std::vector<PGNMatch> PGN::ReadFromFile(const std::string& filename)
	{
		std::ifstream file(filename);
		file.seekg(0, std::ios::end);
		size_t filesize = file.tellg();
		file.seekg(0, std::ios::beg);
		std::string result;
		result.reserve(filesize / sizeof(char));
		result.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		file.close();
		return ReadFromString(result);
	}

	char PGN::GetFileName(File file)
	{
		return (char)('a' + (file - FILE_A));
	}

	char PGN::GetRankName(Rank rank)
	{
		return (char)('1' + (rank - RANK_1));
	}

	File PGN::GetFileFromName(char name)
	{
		return (File)(name - 'a' + FILE_A);
	}

	Rank PGN::GetRankFromName(char name)
	{
		return (Rank)(name - '1' + RANK_1);
	}

	std::string PGN::GetPieceName(Piece piece)
	{
		switch (piece)
		{
		case PIECE_PAWN:
			return "";
		case PIECE_KNIGHT:
			return "N";
		case PIECE_BISHOP:
			return "B";
		case PIECE_ROOK:
			return "R";
		case PIECE_QUEEN:
			return "Q";
		case PIECE_KING:
			return "K";
		default:
			break;
		}
		STORM_ASSERT(false, "Invalid piece");
		return "";
	}

	Piece PGN::GetPieceFromName(char piece)
	{
		switch (piece)
		{
		case 'N':
			return PIECE_KNIGHT;
		case 'B':
			return PIECE_BISHOP;
		case 'R':
			return PIECE_ROOK;
		case 'Q':
			return PIECE_QUEEN;
		case 'K':
			return PIECE_KING;
		default:
			return PIECE_NONE;
		}
		return PIECE_NONE;
	}

	bool PGN::IsCapital(char c)
	{
		return std::isupper(c);
	}

}
