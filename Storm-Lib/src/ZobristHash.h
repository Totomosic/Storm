#pragma once
#include "Types.h"

namespace Storm
{

	extern uint64_t s_PieceOnSquare[COLOR_MAX][PIECE_MAX][SQUARE_MAX];
	extern uint64_t s_BlackToMove;
	extern uint64_t s_CastlingRights[4];
	extern uint64_t s_EnPassantFile[FILE_MAX];

	void InitZobristHash();

	class Position;

	class STORM_API ZobristHash
	{
	public:
		uint64_t Hash;

	public:
		ZobristHash();
		ZobristHash(uint64_t hash);

		void SetFromPosition(const Position& position);

		inline void RemovePieceAt(Color color, Piece piece, SquareIndex square)
		{
			Hash ^= s_PieceOnSquare[color][piece][square];
		}

		inline void AddPieceAt(Color color, Piece piece, SquareIndex square)
		{
			Hash ^= s_PieceOnSquare[color][piece][square];
		}

		inline void FlipTeamToPlay()
		{
			Hash ^= s_BlackToMove;
		}

		inline void RemoveEnPassant(File file)
		{
			Hash ^= s_EnPassantFile[file];
		}

		inline void AddEnPassant(File file)
		{
			Hash ^= s_EnPassantFile[file];
		}

		inline void AddCastleKingside(Color color)
		{
			if (color == COLOR_WHITE)
				Hash ^= s_CastlingRights[0];
			else
				Hash ^= s_CastlingRights[2];
		}

		inline void AddCastleQueenside(Color color)
		{
			if (color == COLOR_WHITE)
				Hash ^= s_CastlingRights[1];
			else
				Hash ^= s_CastlingRights[3];
		}

		inline void RemoveCastleKingside(Color color)
		{
			AddCastleKingside(color);
		}

		inline void RemoveCastleQueenside(Color color)
		{
			AddCastleQueenside(color);
		}

		inline ZobristHash& operator^=(const ZobristHash& right)
		{
			Hash ^= right.Hash;
			return *this;
		}
	};

	inline ZobristHash operator^(const ZobristHash& left, const ZobristHash& right)
	{
		return left.Hash ^ right.Hash;
	}

	inline bool operator==(const ZobristHash& left, const ZobristHash& right)
	{
		return left.Hash == right.Hash;
	}

	inline bool operator!=(const ZobristHash& left, const ZobristHash& right)
	{
		return left.Hash != right.Hash;
	}
}

namespace std
{

	template<>
	struct hash<Storm::ZobristHash>
	{
	public:
		size_t operator()(const Storm::ZobristHash& hash) const
		{
			return hash.Hash;
		}
	};

}
