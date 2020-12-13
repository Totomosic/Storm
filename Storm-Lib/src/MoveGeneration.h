#pragma once
#include "Position.h"
#include "Move.h"

namespace Storm
{

	enum GenerationType
	{
		CAPTURES = 1 << 0,
		QUIETS = 1 << 1,
		ALL = CAPTURES | QUIETS,
	};

	using MoveType = Move;

	namespace Internal
	{
		inline MoveType* AddMoves(BitBoard moves, SquareIndex fromSquare, MoveType* moveList)
		{
			while (moves)
			{
				SquareIndex toSquare = PopLeastSignificantBit(moves);
				*moveList++ = CreateMove(fromSquare, toSquare);
			}
			return moveList;
		}

		template<Color C, GenerationType TYPE>
		inline MoveType* GeneratePawnMoves(const Position& position, MoveType* moveList)
		{
			constexpr Direction Up = C == COLOR_WHITE ? NORTH : SOUTH;
			constexpr BitBoard PromotionMask = RANK_MASKS[GetPromotionRank(C)];
			constexpr BitBoard DoublePushPawns = C == COLOR_WHITE ? RANK_2_BB : RANK_7_BB;
			constexpr Direction AttackDir0 = C == COLOR_WHITE ? NORTH_EAST : SOUTH_EAST;
			constexpr Direction AttackDir1 = C == COLOR_WHITE ? NORTH_WEST : SOUTH_WEST;
			if constexpr (TYPE & QUIETS)
			{
				const BitBoard available = ~position.GetPieces();
				BitBoard pawns = position.GetPieces(C, PIECE_PAWN);
				BitBoard singlePushes = Shift<Up>(pawns) & available;
				BitBoard doublePushes = Shift<Up>(Shift<Up>(pawns & DoublePushPawns) & available) & available;
				BitBoard promotions = singlePushes & PromotionMask;
				singlePushes &= ~PromotionMask;
				while (singlePushes)
				{
					SquareIndex square = PopLeastSignificantBit(singlePushes);
					*moveList++ = CreateMove(SquareBehind(square, C), square);
				}
				while (doublePushes)
				{
					SquareIndex square = PopLeastSignificantBit(doublePushes);
					*moveList++ = CreateMove(SquareBehind(SquareBehind(square, C), C), square);
				}
				while (promotions)
				{
					SquareIndex square = PopLeastSignificantBit(promotions);
					*moveList++ = CreateMove(SquareBehind(square, C), square, PIECE_QUEEN);
					*moveList++ = CreateMove(SquareBehind(square, C), square, PIECE_KNIGHT);
					*moveList++ = CreateMove(SquareBehind(square, C), square, PIECE_ROOK);
					*moveList++ = CreateMove(SquareBehind(square, C), square, PIECE_BISHOP);
				}
			}
			if constexpr (TYPE & CAPTURES)
			{
				BitBoard pawns = position.GetPieces(C, PIECE_PAWN);
				BitBoard enemies = position.GetPieces(OtherColor(C));
				if (position.EnpassantSquare != SQUARE_INVALID)
					enemies |= position.EnpassantSquare;
				while (pawns)
				{
					SquareIndex sq = PopLeastSignificantBit(pawns);
					BitBoard square = GetSquareBB(sq);
					BitBoard attacks = Shift<AttackDir0>(square) | Shift<AttackDir1>(square);
					moveList = AddMoves(attacks & enemies, sq, moveList);
				}
			}
			return moveList;
		}

		template<Color C, GenerationType TYPE, Piece PIECE>
		inline MoveType* GenerateMoves(const Position& position, MoveType* moveList)
		{
			static_assert(PIECE == PIECE_KNIGHT || PIECE == PIECE_BISHOP || PIECE == PIECE_ROOK || PIECE == PIECE_QUEEN);
			BitBoard targetMask = ((TYPE & QUIETS) ? ~position.GetPieces(C) : ZERO_BB) | ((TYPE & CAPTURES) ? position.GetPieces(OtherColor(C)) : ZERO_BB);
			BitBoard pieces = position.GetPieces(C, PIECE);
			if constexpr (PIECE == PIECE_KNIGHT)
			{
				while (pieces)
				{
					SquareIndex square = PopLeastSignificantBit(pieces);
					moveList = AddMoves(GetAttacks<PIECE>(square) & targetMask, square, moveList);
				}
			}
			else
			{
				BitBoard blockers = position.GetPieces();
				while (pieces)
				{
					SquareIndex square = PopLeastSignificantBit(pieces);
					moveList = AddMoves(GetAttacks<PIECE>(square, blockers) & targetMask, square, moveList);
				}
			}
			return moveList;
		}

		template<Color C, GenerationType TYPE>
		inline MoveType* GenerateKingMoves(const Position& position, MoveType* moveList)
		{
			constexpr Rank CastleRank = C == COLOR_WHITE ? RANK_1 : RANK_8;
			BitBoard targetMask = ((TYPE & QUIETS) ? ~position.GetPieces(C) : ZERO_BB) | ((TYPE & CAPTURES) ? position.GetPieces(OtherColor(C)) : ZERO_BB);
			BitBoard attacks = GetAttacks<PIECE_KING>(position.GetKingSquare(C));
			moveList = AddMoves(attacks & targetMask, position.GetKingSquare(C), moveList);

			if constexpr (TYPE & QUIETS)
			{
				// Castling moves - does not check that we are castling through/into check
				// Does check that the squares between king and rook are empty
				if (position.Colors[C].CastleKingSide && !position.SquareOccupied(CreateSquare(FILE_F, CastleRank)) && !position.SquareOccupied(CreateSquare(FILE_G, CastleRank)))
				{
					*moveList++ = CreateMove(CreateSquare(FILE_E, CastleRank), CreateSquare(FILE_G, CastleRank));
				}
				if (position.Colors[C].CastleQueenSide && !position.SquareOccupied(CreateSquare(FILE_D, CastleRank)) && !position.SquareOccupied(CreateSquare(FILE_C, CastleRank)) && !position.SquareOccupied(CreateSquare(FILE_B, CastleRank)))
				{
					*moveList++ = CreateMove(CreateSquare(FILE_E, CastleRank), CreateSquare(FILE_C, CastleRank));
				}
			}
			return moveList;
		}
	}

	// Generate all Pseudo-legal moves according to given GenerationType
	template<Color C, GenerationType TYPE, Piece PIECE>
	inline MoveType* Generate(const Position& position, MoveType* moveList)
	{
		if constexpr (PIECE == PIECE_PAWN)
		{
			return Internal::GeneratePawnMoves<C, TYPE>(position, moveList);
		}
		else if constexpr (PIECE == PIECE_KING)
		{
			return Internal::GenerateKingMoves<C, TYPE>(position, moveList);
		}
		else
		{
			return Internal::GenerateMoves<C, TYPE, PIECE>(position, moveList);
		}
	}

	template<Color C, GenerationType TYPE>
	inline MoveType* GenerateAll(const Position& position, MoveType* moveList)
	{
		moveList = Generate<C, TYPE, PIECE_PAWN>(position, moveList);
		moveList = Generate<C, TYPE, PIECE_KNIGHT>(position, moveList);
		moveList = Generate<C, TYPE, PIECE_BISHOP>(position, moveList);
		moveList = Generate<C, TYPE, PIECE_ROOK>(position, moveList);
		moveList = Generate<C, TYPE, PIECE_QUEEN>(position, moveList);
		moveList = Generate<C, TYPE, PIECE_KING>(position, moveList);
		return moveList;
	}

}
