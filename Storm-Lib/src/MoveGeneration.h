#pragma once
#include "Position.h"
#include "Move.h"
#include "Attacks.h"

#ifdef SWIG
#define STORM_API
#define ValueType int16_t
#endif

namespace Storm
{

    constexpr int MAX_MOVES = 218;

    STORM_API enum GenerationType : uint8_t
    {
        ALL,
        CAPTURES,
        QUIETS,
        EVASIONS,
        EVASION_CAPTURES,
        EVASION_QUIETS,
    };

    constexpr GenerationType operator|(GenerationType a, GenerationType b)
    {
        return GenerationType(uint8_t(a) | uint8_t(b));
    }

    namespace Internal
    {
        template<typename MT>
        inline MT* AddMoves(BitBoard moves, SquareIndex fromSquare, MT* moveList)
        {
            while (moves)
            {
                SquareIndex toSquare = PopLeastSignificantBit(moves);
                *moveList++ = CreateMove(fromSquare, toSquare);
            }
            return moveList;
        }

        template<typename MT>
        inline MT* AddMoves(BitBoard moves, SquareIndex fromSquare, Piece promotionPiece, MT* moveList)
        {
            while (moves)
            {
                SquareIndex toSquare = PopLeastSignificantBit(moves);
                *moveList++ = CreateMove(fromSquare, toSquare, promotionPiece);
            }
            return moveList;
        }

        template<Color C, GenerationType TYPE, typename MT>
        inline MT* GeneratePawnMoves(const Position& position, BitBoard targetSquares, MT* moveList)
        {
            constexpr Direction Up = C == COLOR_WHITE ? NORTH : SOUTH;
            constexpr BitBoard PromotionMask = RANK_MASKS[GetPromotionRank(C)];
            constexpr BitBoard DoublePushPawns = C == COLOR_WHITE ? RANK_2_BB : RANK_7_BB;
            constexpr Direction AttackDir0 = C == COLOR_WHITE ? NORTH_EAST : SOUTH_EAST;
            constexpr Direction AttackDir1 = C == COLOR_WHITE ? NORTH_WEST : SOUTH_WEST;
            if constexpr (TYPE == QUIETS || TYPE == EVASIONS || TYPE == ALL || TYPE == EVASION_QUIETS)
            {
                BitBoard availableSquares = ~position.GetPieces();
                BitBoard pawns = position.GetPieces(C, PIECE_PAWN);
                BitBoard singlePushes = Shift<Up>(pawns) & availableSquares & targetSquares;
                BitBoard doublePushes =
                  Shift<Up>(Shift<Up>(pawns & DoublePushPawns) & availableSquares) & availableSquares & targetSquares;
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
            if constexpr (TYPE == CAPTURES || TYPE == ALL || TYPE == EVASIONS || TYPE == EVASION_CAPTURES)
            {
                BitBoard pawns = position.GetPieces(C, PIECE_PAWN);
                BitBoard enemies = position.GetPieces(OtherColor(C)) & targetSquares;
                // Enpassant is rare enough that we don't care if we generate too many
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

        template<Color C, Piece PIECE, typename MT>
        inline MT* GenerateMoves(const Position& position, BitBoard availableSquares, MT* moveList)
        {
            static_assert(
              PIECE == PIECE_KNIGHT || PIECE == PIECE_BISHOP || PIECE == PIECE_ROOK || PIECE == PIECE_QUEEN);
            BitBoard pieces = position.GetPieces(C, PIECE);
            if constexpr (PIECE == PIECE_KNIGHT)
            {
                while (pieces)
                {
                    SquareIndex square = PopLeastSignificantBit(pieces);
                    moveList = AddMoves(GetAttacks<PIECE>(square) & availableSquares, square, moveList);
                }
            }
            else
            {
                BitBoard blockers = position.GetPieces();
                while (pieces)
                {
                    SquareIndex square = PopLeastSignificantBit(pieces);
                    moveList = AddMoves(GetAttacks<PIECE>(square, blockers) & availableSquares, square, moveList);
                }
            }
            return moveList;
        }

        template<Color C, typename MT>
        inline MT* GenerateKingMoves(const Position& position, BitBoard availableSquares, MT* moveList)
        {
            constexpr Rank CastleRank = C == COLOR_WHITE ? RANK_1 : RANK_8;
            BitBoard attacks = GetAttacks<PIECE_KING>(position.GetKingSquare(C));
            moveList = AddMoves(attacks & availableSquares, position.GetKingSquare(C), moveList);

            constexpr SquareIndex KingsideCastleSquare = CreateSquare(FILE_G, CastleRank);
            constexpr SquareIndex QueensideCastleSquare = CreateSquare(FILE_C, CastleRank);

            // Castling moves - does not check that we are castling through/into check
            // Does check that the squares between king and rook are empty
            if (position.Colors[C].CastleKingSide && (availableSquares & KingsideCastleSquare) &&
                !position.SquareOccupied(CreateSquare(FILE_F, CastleRank)) &&
                !position.SquareOccupied(CreateSquare(FILE_G, CastleRank)))
            {
                *moveList++ = CreateMove(CreateSquare(FILE_E, CastleRank), KingsideCastleSquare, CASTLE);
            }
            if (position.Colors[C].CastleQueenSide && (availableSquares & QueensideCastleSquare) &&
                !position.SquareOccupied(CreateSquare(FILE_D, CastleRank)) &&
                !position.SquareOccupied(CreateSquare(FILE_C, CastleRank)) &&
                !position.SquareOccupied(CreateSquare(FILE_B, CastleRank)))
            {
                *moveList++ = CreateMove(CreateSquare(FILE_E, CastleRank), QueensideCastleSquare, CASTLE);
            }
            return moveList;
        }
    }

    // Generate all Pseudo-legal moves according to given GenerationType
    template<Color C, GenerationType TYPE, Piece PIECE, typename MT>
    inline MT* Generate(const Position& position, BitBoard availableSquares, MT* moveList)
    {
        if constexpr (PIECE == PIECE_PAWN)
            return Internal::GeneratePawnMoves<C, TYPE, MT>(position, availableSquares, moveList);
        else if constexpr (PIECE == PIECE_KING)
            return Internal::GenerateKingMoves<C, MT>(position, availableSquares, moveList);
        else
            return Internal::GenerateMoves<C, PIECE, MT>(position, availableSquares, moveList);
    }

    template<Color C, GenerationType TYPE, typename MT>
    inline MT* GenerateAll(const Position& position, MT* moveList)
    {
        if ((TYPE == EVASIONS || TYPE == EVASION_CAPTURES || TYPE == EVASION_QUIETS) &&
            MoreThanOne(position.GetCheckers()))
        {
            STORM_ASSERT(position.InCheck(), "Must be in check");
            BitBoard availableSquares = ZERO_BB;
            if constexpr (TYPE == EVASIONS || TYPE == EVASION_CAPTURES)
            {
                availableSquares |= position.GetPieces(OtherColor(C));
            }
            if constexpr (TYPE == EVASIONS || TYPE == EVASION_QUIETS)
            {
                availableSquares |= ~position.GetPieces();
            }
            moveList = Generate<C, TYPE, PIECE_KING, MT>(position, availableSquares, moveList);
        }
        else
        {
            BitBoard availableSquares = ZERO_BB;
            BitBoard availableKingSquares = ZERO_BB;
            if constexpr (TYPE == CAPTURES)
            {
                STORM_ASSERT(!position.InCheck(), "Cannot be in check");
                availableSquares |= position.GetPieces(OtherColor(C));
                availableKingSquares |= position.GetPieces(OtherColor(C));
            }
            if constexpr (TYPE == QUIETS)
            {
                STORM_ASSERT(!position.InCheck(), "Cannot be in check");
                availableSquares |= ~position.GetPieces();
                availableKingSquares |= ~position.GetPieces();
            }
            if constexpr (TYPE == EVASIONS)
            {
                STORM_ASSERT(position.InCheck() && !MoreThanOne(position.GetCheckers()), "Must be in check");
                SquareIndex checker = LeastSignificantBit(position.GetCheckers());
                availableSquares |=
                  (GetBitBoardBetween(position.GetKingSquare(C), checker) & ~position.GetPieces(C)) | checker;
                availableKingSquares |= ~position.GetPieces(C);
            }
            if constexpr (TYPE == EVASION_CAPTURES)
            {
                STORM_ASSERT(position.InCheck() && !MoreThanOne(position.GetCheckers()), "Must be in check");
                SquareIndex checker = LeastSignificantBit(position.GetCheckers());
                availableSquares |=
                  (GetBitBoardBetween(position.GetKingSquare(C), checker) & position.GetPieces(OtherColor(C))) |
                  checker;
                availableKingSquares |= position.GetPieces(OtherColor(C));
            }
            if constexpr (TYPE == EVASION_QUIETS)
            {
                STORM_ASSERT(position.InCheck() && !MoreThanOne(position.GetCheckers()), "Must be in check");
                SquareIndex checker = LeastSignificantBit(position.GetCheckers());
                availableSquares |= GetBitBoardBetween(position.GetKingSquare(C), checker) & ~position.GetPieces();
                availableKingSquares |= ~position.GetPieces();
            }
            if constexpr (TYPE == ALL)
            {
                if (position.InCheck())
                {
                    if (MoreThanOne(position.GetCheckers()))
                    {
                        moveList = Generate<C, TYPE, PIECE_KING, MT>(position, ~position.GetPieces(C), moveList);
                        return moveList;
                    }
                    else
                    {
                        SquareIndex checker = LeastSignificantBit(position.GetCheckers());
                        availableSquares |= GetBitBoardBetween(position.GetKingSquare(C), checker) | checker;
                        availableKingSquares |= ~position.GetPieces(C);
                    }
                }
                else
                {
                    availableSquares |= ~position.GetPieces(C);
                    availableKingSquares |= ~position.GetPieces(C);
                }
            }

            moveList = Generate<C, TYPE, PIECE_PAWN, MT>(position, availableSquares, moveList);
            moveList = Generate<C, TYPE, PIECE_KNIGHT, MT>(position, availableSquares, moveList);
            moveList = Generate<C, TYPE, PIECE_BISHOP, MT>(position, availableSquares, moveList);
            moveList = Generate<C, TYPE, PIECE_ROOK, MT>(position, availableSquares, moveList);
            moveList = Generate<C, TYPE, PIECE_QUEEN, MT>(position, availableSquares, moveList);
            moveList = Generate<C, TYPE, PIECE_KING, MT>(position, availableKingSquares, moveList);
        }
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
        inline MoveList(MT* buffer) : m_Start(buffer), m_End(buffer) {}

        inline const MT* GetStart() const
        {
            return m_Start;
        }

        inline MT* GetStart()
        {
            return m_Start;
        }

        inline const MT* begin() const
        {
            return m_Start;
        }

        inline const MT* end() const
        {
            return m_End;
        }

        inline MT* begin()
        {
            return m_Start;
        }

        inline MT* end()
        {
            return m_End;
        }

        inline size_t Size() const
        {
            return m_End - m_Start;
        }

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
