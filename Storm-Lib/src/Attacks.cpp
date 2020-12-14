#include "Attacks.h"

namespace Storm
{

	static bool s_RaysInitialized = false;
	static bool s_AttacksInitialized = false;

	static BitBoard s_Rays[DIRECTION_MAX][SQUARE_MAX];

	template<Direction D>
	static BitBoard ShiftMultiple(BitBoard board, int count)
	{
		BitBoard result = board;
		for (int i = 0; i < count; i++)
			result = Shift<D>(result);
		return result;
	}

	BitBoard s_NonSlidingAttacks[COLOR_MAX][PIECE_MAX][SQUARE_MAX];
	BitBoard s_SlidingAttacks[COLOR_MAX][PIECE_MAX][SQUARE_MAX];

	BitBoard s_RookMasks[SQUARE_MAX] = { ZERO_BB };
	BitBoard s_BishopMasks[SQUARE_MAX] = { ZERO_BB };

	BitBoard s_RookTable[SQUARE_MAX][4096] = { { ZERO_BB } };
	BitBoard s_BishopTable[SQUARE_MAX][1024] = { { ZERO_BB } };

    static BitBoard GetBlockersFromIndex(int index, BitBoard mask)
    {
        BitBoard blockers = ZERO_BB;
        int bits = Popcount(mask);
        for (int i = 0; i < bits; i++)
        {
            int bitPosition = PopLeastSignificantBit(mask);
            if (index & (1 << i))
            {
                blockers |= BitBoard(1ULL << bitPosition);
            }
        }
        return blockers;
    }

	void InitRays()
	{
		if (!s_RaysInitialized)
		{
			for (SquareIndex square = a1; square < SQUARE_MAX; square++)
			{
				s_Rays[NORTH][square] = BitBoard(0x0101010101010100ULL << square);
				s_Rays[SOUTH][square] = BitBoard(0x0080808080808080ULL >> (63 - square));
				s_Rays[EAST][square] = BitBoard(2 * ((1ULL << (square | 7)) - (1ULL << square)));
				s_Rays[WEST][square] = BitBoard((1ULL << square) - (1ULL << (square & 56)));
				s_Rays[NORTH_WEST][square] = ShiftMultiple<WEST>(BitBoard(0x102040810204000ULL), 7 - FileOf(square)) << (RankOf(square) * 8);
				s_Rays[NORTH_EAST][square] = ShiftMultiple<EAST>(BitBoard(0x8040201008040200ULL), FileOf(square)) << (RankOf(square) * 8);
				s_Rays[SOUTH_WEST][square] = ShiftMultiple<WEST>(BitBoard(0x40201008040201ULL), 7 - FileOf(square)) >> ((7 - RankOf(square)) * 8);
				s_Rays[SOUTH_EAST][square] = ShiftMultiple<EAST>(BitBoard(0x2040810204080ULL), FileOf(square)) >> ((7 - RankOf(square)) * 8);
			}
			s_RaysInitialized = true;
		}
	}

    void InitPawnAttacks()
    {
        for (SquareIndex square = a1; square < SQUARE_MAX; square++)
        {
            BitBoard start = GetSquareBB(square);
            BitBoard whiteAttack = ((start << 9) & ~FILE_A_BB) | ((start << 7) & ~FILE_H_BB);
            BitBoard blackAttack = ((start >> 9) & ~FILE_H_BB) | ((start >> 7) & ~FILE_A_BB);
            s_NonSlidingAttacks[COLOR_WHITE][PIECE_PAWN][square] = whiteAttack;
            s_NonSlidingAttacks[COLOR_BLACK][PIECE_PAWN][square] = blackAttack;
        }
    }

    void InitKnightAttacks()
    {
        for (SquareIndex square = a1; square < SQUARE_MAX; square++)
        {
            BitBoard start = GetSquareBB(square);
            BitBoard attack = (((start << 15) | (start >> 17)) & ~FILE_H_BB) |    // Left 1
                (((start >> 15) | (start << 17)) & ~FILE_A_BB) |                  // Right 1
                (((start << 6) | (start >> 10)) & ~(FILE_G_BB | FILE_H_BB)) |   // Left 2
                (((start >> 6) | (start << 10)) & ~(FILE_A_BB | FILE_B_BB));    // Right 2
            s_NonSlidingAttacks[COLOR_WHITE][PIECE_KNIGHT][square] = attack;
            s_NonSlidingAttacks[COLOR_BLACK][PIECE_KNIGHT][square] = attack;
        }
    }

    void InitKingAttacks()
    {
        for (SquareIndex square = a1; square < SQUARE_MAX; square++)
        {
            BitBoard start = GetSquareBB(square);
            BitBoard attack = (((start << 7) | (start >> 9) | (start >> 1)) & (~FILE_H_BB)) |
                (((start << 9) | (start >> 7) | (start << 1)) & (~FILE_A_BB)) |
                ((start >> 8) | (start << 8));
            s_NonSlidingAttacks[COLOR_WHITE][PIECE_KING][square] = attack;
            s_NonSlidingAttacks[COLOR_BLACK][PIECE_KING][square] = attack;
        }
    }

    void InitRookMasks()
    {
        for (SquareIndex square = a1; square < SQUARE_MAX; square++)
        {
            s_RookMasks[square] = (GetRay(NORTH, square) & ~RANK_8_BB) | (GetRay(SOUTH, square) & ~RANK_1_BB) | (GetRay(EAST, square) & ~FILE_H_BB) | (GetRay(WEST, square) & ~FILE_A_BB);
        }
    }

    void InitBishopMasks()
    {
        constexpr BitBoard notEdges = ~(FILE_A_BB | FILE_H_BB | RANK_1_BB | RANK_8_BB);
        for (SquareIndex square = a1; square < SQUARE_MAX; square++)
        {
            s_BishopMasks[square] = ((GetRay(NORTH_EAST, square)) | (GetRay(SOUTH_EAST, square)) | (GetRay(SOUTH_WEST, square)) | (GetRay(NORTH_WEST, square))) & notEdges;
        }
    }

    BitBoard GetRookAttacksSlow(SquareIndex square, BitBoard blockers)
    {
        BitBoard attacks = ZERO_BB;
        BitBoard northRay = GetRay(NORTH, square);
        attacks |= northRay;
        if (northRay & blockers)
        {
            attacks &= ~(GetRay(NORTH, LeastSignificantBit(northRay & blockers)));
        }

        BitBoard southRay = GetRay(SOUTH, square);
        attacks |= southRay;
        if (southRay & blockers)
        {
            attacks &= ~(GetRay(SOUTH, MostSignificantBit(southRay & blockers)));
        }

        BitBoard eastRay = GetRay(EAST, square);
        attacks |= eastRay;
        if (eastRay & blockers)
        {
            attacks &= ~(GetRay(EAST, LeastSignificantBit(eastRay & blockers)));
        }

        BitBoard westRay = GetRay(WEST, square);
        attacks |= westRay;
        if (westRay & blockers)
        {
            attacks &= ~(GetRay(WEST, MostSignificantBit(westRay & blockers)));
        }
        return attacks;
    }

    BitBoard GetBishopAttacksSlow(SquareIndex square, BitBoard blockers)
    {
        BitBoard attacks = ZERO_BB;
        BitBoard northEastRay = GetRay(NORTH_EAST, square);
        attacks |= northEastRay;
        if (northEastRay & blockers)
        {
            attacks &= ~(GetRay(NORTH_EAST, LeastSignificantBit(northEastRay & blockers)));
        }

        BitBoard northWestRay = GetRay(NORTH_WEST, square);
        attacks |= northWestRay;
        if (northWestRay & blockers)
        {
            attacks &= ~(GetRay(NORTH_WEST, LeastSignificantBit(northWestRay & blockers)));
        }

        BitBoard southEastRay = GetRay(SOUTH_EAST, square);
        attacks |= southEastRay;
        if (southEastRay & blockers)
        {
            attacks &= ~(GetRay(SOUTH_EAST, MostSignificantBit(southEastRay & blockers)));
        }

        BitBoard southWestRay = GetRay(SOUTH_WEST, square);
        attacks |= southWestRay;
        if (southWestRay & blockers)
        {
            attacks &= ~(GetRay(SOUTH_WEST, MostSignificantBit(southWestRay & blockers)));
        }
        return attacks;
    }

    void InitRookMagicTable()
    {
        for (SquareIndex square = a1; square < FILE_MAX * RANK_MAX; square++)
        {
            int maxBlockers = 1 << s_RookIndexBits[square];
            for (int blockerIndex = 0; blockerIndex < maxBlockers; blockerIndex++)
            {
                BitBoard blockers = GetBlockersFromIndex(blockerIndex, s_RookMasks[square]);
                s_RookTable[square][(blockers * s_RookMagics[square]) >> (int)(64 - s_RookIndexBits[square])] = GetRookAttacksSlow(square, blockers);
            }
        }
    }

    void InitBishopMagicTable()
    {
        for (SquareIndex square = a1; square < FILE_MAX * RANK_MAX; square++)
        {
            int maxBlockers = 1 << s_BishopIndexBits[square];
            for (int blockerIndex = 0; blockerIndex < maxBlockers; blockerIndex++)
            {
                BitBoard blockers = GetBlockersFromIndex(blockerIndex, s_BishopMasks[square]);
                s_BishopTable[square][(blockers * s_BishopMagics[square]) >> (int)(64 - s_BishopIndexBits[square])] = GetBishopAttacksSlow(square, blockers);
            }
        }
    }

    BitBoard s_Lines[FILE_MAX * RANK_MAX][FILE_MAX * RANK_MAX];

    void InitLines()
    {
        for (SquareIndex s1 = a1; s1 < FILE_MAX * RANK_MAX; s1++)
        {
            for (SquareIndex s2 = a1; s2 < FILE_MAX * RANK_MAX; s2++)
            {
                if (GetAttacks<PIECE_ROOK>(s1, ZERO_BB) & s2)
                    s_Lines[s1][s2] = (GetAttacks<PIECE_ROOK>(s1, ZERO_BB) & GetAttacks<PIECE_ROOK>(s2, ZERO_BB)) | s1 | s2;
                if (GetAttacks<PIECE_BISHOP>(s1, ZERO_BB) & s2)
                    s_Lines[s1][s2] = (GetAttacks<PIECE_BISHOP>(s1, ZERO_BB) & GetAttacks<PIECE_BISHOP>(s2, ZERO_BB)) | s1 | s2;
            }
        }
    }

	void InitAttacks()
	{
		if (!s_AttacksInitialized)
		{
            InitPawnAttacks();
            InitKnightAttacks();
            InitKingAttacks();
            InitRookMasks();
            InitBishopMasks();
            InitRookMagicTable();
            InitBishopMagicTable();
            InitLines();
			s_AttacksInitialized = true;
		}
	}

	BitBoard GetRay(Direction direction, SquareIndex fromSquare)
	{
		return s_Rays[direction][fromSquare];
	}

}
