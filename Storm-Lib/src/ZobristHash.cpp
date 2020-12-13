#include "ZobristHash.h"
#include "Position.h"
#include "BitBoard.h"
#include <random>

namespace Storm
{

	static bool s_Initialized = false;

	static constexpr uint32_t s_Seed = 0x412FDA8F;//0xDABDABDA;

	uint64_t s_PieceOnSquare[COLOR_MAX][PIECE_MAX][FILE_MAX * RANK_MAX];
	uint64_t s_BlackToMove;
	uint64_t s_CastlingRights[4];
	uint64_t s_EnPassantFile[FILE_MAX];

	void InitZobristHash()
	{
		if (!s_Initialized)
		{
			std::mt19937 mt;
			mt.seed(s_Seed);
			std::uniform_int_distribution<uint64_t> dist;

			for (Piece piece = PIECE_PAWN; piece < PIECE_MAX; piece++)
			{
				for (SquareIndex index = a1; index < FILE_MAX * RANK_MAX; index++)
				{
					s_PieceOnSquare[COLOR_WHITE][piece][index] = dist(mt);
					s_PieceOnSquare[COLOR_BLACK][piece][index] = dist(mt);
				}
			}
			s_BlackToMove = dist(mt);
			s_CastlingRights[0] = dist(mt);
			s_CastlingRights[1] = dist(mt);
			s_CastlingRights[2] = dist(mt);
			s_CastlingRights[3] = dist(mt);
			for (File file = FILE_A; file < FILE_MAX; file++)
			{
				s_EnPassantFile[file] = dist(mt);
			}
			s_Initialized = true;
		}
	}

	ZobristHash operator^(const ZobristHash& left, const ZobristHash& right)
	{
		return left.Hash ^ right.Hash;
	}

	bool operator==(const ZobristHash& left, const ZobristHash& right)
	{
		return left.Hash == right.Hash;
	}

	bool operator!=(const ZobristHash& left, const ZobristHash& right)
	{
		return left.Hash != right.Hash;
	}

	ZobristHash::ZobristHash()
		: Hash(0ULL)
	{
	}

	ZobristHash::ZobristHash(uint64_t hash)
		: Hash(hash)
	{
	}

	void ZobristHash::SetFromPosition(const Position& position)
	{
		Hash = 0ULL;
		for (Piece piece = PIECE_START; piece < PIECE_MAX; piece++)
		{
			BitBoard whitePieces = position.GetPieces(COLOR_WHITE, piece);
			while (whitePieces)
			{
				SquareIndex index = PopLeastSignificantBit(whitePieces);
				AddPieceAt(COLOR_WHITE, piece, index);
			}
			BitBoard blackPieces = position.GetPieces(COLOR_BLACK, piece);
			while (blackPieces)
			{
				SquareIndex index = PopLeastSignificantBit(blackPieces);
				AddPieceAt(COLOR_BLACK, piece, index);
			}
		}
		if (position.EnpassantSquare != SQUARE_INVALID)
		{
			AddEnPassant(FileOf(position.EnpassantSquare));
		}
		if (position.ColorToMove == COLOR_BLACK)
		{
			FlipTeamToPlay();
		}
		if (position.Colors[COLOR_WHITE].CastleKingSide)
			AddCastleKingside(COLOR_WHITE);
		if (position.Colors[COLOR_WHITE].CastleQueenSide)
			AddCastleQueenside(COLOR_WHITE);
		if (position.Colors[COLOR_BLACK].CastleKingSide)
			AddCastleKingside(COLOR_BLACK);
		if (position.Colors[COLOR_BLACK].CastleQueenSide)
			AddCastleQueenside(COLOR_BLACK);
	}

	void ZobristHash::RemovePieceAt(Color color, Piece piece, SquareIndex square)
	{
		Hash ^= s_PieceOnSquare[color][piece][square];
	}

	void ZobristHash::AddPieceAt(Color color, Piece piece, SquareIndex square)
	{
		Hash ^= s_PieceOnSquare[color][piece][square];
	}

	void ZobristHash::FlipTeamToPlay()
	{
		Hash ^= s_BlackToMove;
	}

	void ZobristHash::RemoveEnPassant(File file)
	{
		Hash ^= s_EnPassantFile[file];
	}

	void ZobristHash::AddEnPassant(File file)
	{
		Hash ^= s_EnPassantFile[file];
	}

	void ZobristHash::AddCastleKingside(Color color)
	{
		if (color == COLOR_WHITE)
			Hash ^= s_CastlingRights[0];
		else
			Hash ^= s_CastlingRights[2];
	}

	void ZobristHash::AddCastleQueenside(Color color)
	{
		if (color == COLOR_WHITE)
			Hash ^= s_CastlingRights[1];
		else
			Hash ^= s_CastlingRights[3];
	}

	void ZobristHash::RemoveCastleKingside(Color color)
	{
		AddCastleKingside(color);
	}

	void ZobristHash::RemoveCastleQueenside(Color color)
	{
		AddCastleQueenside(color);
	}

	ZobristHash& ZobristHash::operator^=(const ZobristHash& right)
	{
		Hash ^= right.Hash;
		return *this;
	}

}
