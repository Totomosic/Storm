#pragma once
#include "Types.h"

namespace Storm
{

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
		void RemovePieceAt(Color color, Piece piece, SquareIndex square);
		void AddPieceAt(Color color, Piece piece, SquareIndex square);
		void FlipTeamToPlay();
		void RemoveEnPassant(File file);
		void AddEnPassant(File file);

		void AddCastleKingside(Color color);
		void AddCastleQueenside(Color color);
		void RemoveCastleKingside(Color color);
		void RemoveCastleQueenside(Color color);

		friend ZobristHash operator^(const ZobristHash& left, const ZobristHash& right);
		ZobristHash& operator^=(const ZobristHash& right);
		friend bool operator==(const ZobristHash& left, const ZobristHash& right);
		friend bool operator!=(const ZobristHash& left, const ZobristHash& right);
	};

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
