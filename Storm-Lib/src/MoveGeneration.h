#pragma once
#include "Position.h"
#include "Move.h"
#include "Attacks.h"

namespace Storm
{

	constexpr int MAX_MOVES = 218;

	enum GenerationType
	{
		CAPTURES = 1 << 0,
		QUIETS = 1 << 1,
		ALL = CAPTURES | QUIETS,
	};

	namespace Internal
	{
		inline Move* AddMoves(BitBoard moves, SquareIndex fromSquare, Move* moveList)
		{
			while (moves)
			{
				SquareIndex toSquare = PopLeastSignificantBit(moves);
				*moveList++ = CreateMove(fromSquare, toSquare);
			}
			return moveList;
		}

		inline Move* AddMoves(BitBoard moves, SquareIndex fromSquare, Piece promotionPiece, Move* moveList)
		{
			while (moves)
			{
				SquareIndex toSquare = PopLeastSignificantBit(moves);
				*moveList++ = CreateMove(fromSquare, toSquare, promotionPiece);
			}
			return moveList;
		}

		template<Color C, GenerationType TYPE, typename MT>
		inline MT* GeneratePawnMoves(const Position& position, MT* moveList)
		{
			constexpr Direction Up = C == COLOR_WHITE ? NORTH : SOUTH;
			constexpr BitBoard PromotionMask = RANK_MASKS[GetPromotionRank(C)];
			constexpr BitBoard DoublePushPawns = C == COLOR_WHITE ? RANK_2_BB : RANK_7_BB;
			constexpr Direction AttackDir0 = C == COLOR_WHITE ? NORTH_EAST : SOUTH_EAST;
			constexpr Direction AttackDir1 = C == COLOR_WHITE ? NORTH_WEST : SOUTH_WEST;
			if constexpr (bool(TYPE & QUIETS))
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
			if constexpr (bool(TYPE & CAPTURES))
			{
				BitBoard pawns = position.GetPieces(C, PIECE_PAWN);
				BitBoard enemies = position.GetPieces(OtherColor(C));
				if (position.EnpassantSquare != SQUARE_INVALID)
					enemies |= position.EnpassantSquare;
				while (pawns)
				{
					SquareIndex sq = PopLeastSignificantBit(pawns);
					BitBoard square = GetSquareBB(sq);
					BitBoard attacks = (Shift<AttackDir0>(square) | Shift<AttackDir1>(square)) & enemies;
					if (attacks & PromotionMask)
					{
						moveList = AddMoves(attacks, sq, PIECE_QUEEN, moveList);
						moveList = AddMoves(attacks, sq, PIECE_KNIGHT, moveList);
						moveList = AddMoves(attacks, sq, PIECE_ROOK, moveList);
						moveList = AddMoves(attacks, sq, PIECE_BISHOP, moveList);
					}
					else
						moveList = AddMoves(attacks, sq, moveList);
				}
			}
			return moveList;
		}

		template<Color C, GenerationType TYPE, Piece PIECE, typename MT>
		inline MT* GenerateMoves(const Position& position, MT* moveList)
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

		template<Color C, GenerationType TYPE, typename MT>
		inline MT* GenerateKingMoves(const Position& position, MT* moveList)
		{
			constexpr Rank CastleRank = C == COLOR_WHITE ? RANK_1 : RANK_8;
			BitBoard targetMask = ((TYPE & QUIETS) ? ~position.GetPieces(C) : ZERO_BB) | ((TYPE & CAPTURES) ? position.GetPieces(OtherColor(C)) : ZERO_BB);
			BitBoard attacks = GetAttacks<PIECE_KING>(position.GetKingSquare(C));
			moveList = AddMoves(attacks & targetMask, position.GetKingSquare(C), moveList);

			if constexpr (bool(TYPE & QUIETS))
			{
				// Castling moves - does not check that we are castling through/into check
				// Does check that the squares between king and rook are empty
				if (position.Colors[C].CastleKingSide && !position.SquareOccupied(CreateSquare(FILE_F, CastleRank)) && !position.SquareOccupied(CreateSquare(FILE_G, CastleRank)))
				{
					*moveList++ = CreateMove(CreateSquare(FILE_E, CastleRank), CreateSquare(FILE_G, CastleRank), CASTLE);
				}
				if (position.Colors[C].CastleQueenSide && !position.SquareOccupied(CreateSquare(FILE_D, CastleRank)) && !position.SquareOccupied(CreateSquare(FILE_C, CastleRank)) && !position.SquareOccupied(CreateSquare(FILE_B, CastleRank)))
				{
					*moveList++ = CreateMove(CreateSquare(FILE_E, CastleRank), CreateSquare(FILE_C, CastleRank), CASTLE);
				}
			}
			return moveList;
		}
	}

	// Generate all Pseudo-legal moves according to given GenerationType
	template<Color C, GenerationType TYPE, Piece PIECE, typename MT>
	inline MT* Generate(const Position& position, MT* moveList)
	{
		if constexpr (PIECE == PIECE_PAWN)
		{
			return Internal::GeneratePawnMoves<C, TYPE, MT>(position, moveList);
		}
		else if constexpr (PIECE == PIECE_KING)
		{
			return Internal::GenerateKingMoves<C, TYPE, MT>(position, moveList);
		}
		else
		{
			return Internal::GenerateMoves<C, TYPE, PIECE, MT>(position, moveList);
		}
	}

	template<Color C, GenerationType TYPE, typename MT>
	inline MT* GenerateAll(const Position& position, MT* moveList)
	{
		moveList = Generate<C, TYPE, PIECE_PAWN, MT>(position, moveList);
		moveList = Generate<C, TYPE, PIECE_KNIGHT, MT>(position, moveList);
		moveList = Generate<C, TYPE, PIECE_BISHOP, MT>(position, moveList);
		moveList = Generate<C, TYPE, PIECE_ROOK, MT>(position, moveList);
		moveList = Generate<C, TYPE, PIECE_QUEEN, MT>(position, moveList);
		moveList = Generate<C, TYPE, PIECE_KING, MT>(position, moveList);
		return moveList;
	}

	template<Color C, GenerationType TYPE, typename MT>
	inline MT* GenerateAllLegal(const Position& position, MT* moveList)
	{
		MT* end = GenerateAll<C, TYPE, MT>(position, moveList);
		MT* it = moveList;

		while (it != end)
		{
			if (position.IsLegal(*it))
				*moveList++ = *it;
			it++;
		}
		return moveList;
	}

	template<typename MT>
	class STORM_API MoveList
	{
	private:
		MT* m_Start;
		MT* m_End;

	public:
		inline MoveList(MT* buffer)
			: m_Start(buffer), m_End(buffer)
		{
		}

		inline const MT* GetStart() const { return m_Start; }
		inline MT* GetStart() { return m_Start; }

		inline const MT* begin() const { return m_Start; }
		inline const MT* end() const { return m_End; }
		inline MT* begin() { return m_Start; }
		inline MT* end() { return m_End; }

		inline size_t Size() const { return m_End - m_Start; }

		inline void Fill(MT* end)
		{
			m_End = end;
		}

		template<GenerationType TYPE>
		inline void FillLegal(const Position& position)
		{
			if (position.ColorToMove == COLOR_WHITE)
				Fill(GenerateAllLegal<COLOR_WHITE, TYPE>(position, GetStart()));
			else
				Fill(GenerateAllLegal<COLOR_BLACK, TYPE>(position, GetStart()));
		}
	};

}
