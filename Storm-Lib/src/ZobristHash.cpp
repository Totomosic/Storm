#include "ZobristHash.h"
#include "Position.h"
#include "Bitboard.h"
#include <random>

namespace Storm
{

	static bool s_Initialized = false;

	static constexpr uint32_t s_Seed = 0x412FDA8F;//0xDABDABDA;

	uint64_t s_PieceOnSquare[COLOR_MAX][PIECE_MAX][SQUARE_MAX];
	uint64_t s_BlackToMove;
	uint64_t s_CastlingRights[4];
	uint64_t s_EnPassantFile[FILE_MAX];

	static uint64_t Rand64() {

		// http://vigna.di.unimi.it/ftp/papers/xorshift.pdf

		static uint64_t seed = 1070372ull;

		seed ^= seed >> 12;
		seed ^= seed << 25;
		seed ^= seed >> 27;

		return seed * 2685821657736338717ull;
	}

	void InitZobristHash()
	{
		if (!s_Initialized)
		{
			std::mt19937 mt;
			mt.seed(s_Seed);
			std::uniform_int_distribution<uint64_t> dist;

			for (Piece piece = PIECE_PAWN; piece < PIECE_MAX; piece++)
			{
				for (SquareIndex index = a1; index < SQUARE_MAX; index++)
				{
					s_PieceOnSquare[COLOR_WHITE][piece][index] = Rand64();
					s_PieceOnSquare[COLOR_BLACK][piece][index] = Rand64();
				}
			}
			s_BlackToMove = Rand64();
			s_CastlingRights[0] = Rand64();
			s_CastlingRights[1] = Rand64();
			s_CastlingRights[2] = Rand64();
			s_CastlingRights[3] = Rand64();
			for (File file = FILE_A; file < FILE_MAX; file++)
			{
				s_EnPassantFile[file] = Rand64();
			}
			s_Initialized = true;
		}
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

}
