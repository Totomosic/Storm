#pragma once
#include "Position.h"

namespace Storm
{

    constexpr uint64_t s_RookMagics[SQUARE_MAX] = {
        0xa8002c000108020ULL, 0x6c00049b0002001ULL, 0x100200010090040ULL, 0x2480041000800801ULL, 0x280028004000800ULL,
        0x900410008040022ULL, 0x280020001001080ULL, 0x2880002041000080ULL, 0xa000800080400034ULL, 0x4808020004000ULL,
        0x2290802004801000ULL, 0x411000d00100020ULL, 0x402800800040080ULL, 0xb000401004208ULL, 0x2409000100040200ULL,
        0x1002100004082ULL, 0x22878001e24000ULL, 0x1090810021004010ULL, 0x801030040200012ULL, 0x500808008001000ULL,
        0xa08018014000880ULL, 0x8000808004000200ULL, 0x201008080010200ULL, 0x801020000441091ULL, 0x800080204005ULL,
        0x1040200040100048ULL, 0x120200402082ULL, 0xd14880480100080ULL, 0x12040280080080ULL, 0x100040080020080ULL,
        0x9020010080800200ULL, 0x813241200148449ULL, 0x491604001800080ULL, 0x100401000402001ULL, 0x4820010021001040ULL,
        0x400402202000812ULL, 0x209009005000802ULL, 0x810800601800400ULL, 0x4301083214000150ULL, 0x204026458e001401ULL,
        0x40204000808000ULL, 0x8001008040010020ULL, 0x8410820820420010ULL, 0x1003001000090020ULL, 0x804040008008080ULL,
        0x12000810020004ULL, 0x1000100200040208ULL, 0x430000a044020001ULL, 0x280009023410300ULL, 0xe0100040002240ULL,
        0x200100401700ULL, 0x2244100408008080ULL, 0x8000400801980ULL, 0x2000810040200ULL, 0x8010100228810400ULL,
        0x2000009044210200ULL, 0x4080008040102101ULL, 0x40002080411d01ULL, 0x2005524060000901ULL, 0x502001008400422ULL,
        0x489a000810200402ULL, 0x1004400080a13ULL, 0x4000011008020084ULL, 0x26002114058042ULL
    };

    constexpr uint64_t s_BishopMagics[SQUARE_MAX] = {
        0x89a1121896040240ULL, 0x2004844802002010ULL, 0x2068080051921000ULL, 0x62880a0220200808ULL, 0x4042004000000ULL,
        0x100822020200011ULL, 0xc00444222012000aULL, 0x28808801216001ULL, 0x400492088408100ULL, 0x201c401040c0084ULL,
        0x840800910a0010ULL, 0x82080240060ULL, 0x2000840504006000ULL, 0x30010c4108405004ULL, 0x1008005410080802ULL,
        0x8144042209100900ULL, 0x208081020014400ULL, 0x4800201208ca00ULL, 0xf18140408012008ULL, 0x1004002802102001ULL,
        0x841000820080811ULL, 0x40200200a42008ULL, 0x800054042000ULL, 0x88010400410c9000ULL, 0x520040470104290ULL,
        0x1004040051500081ULL, 0x2002081833080021ULL, 0x400c00c010142ULL, 0x941408200c002000ULL, 0x658810000806011ULL,
        0x188071040440a00ULL, 0x4800404002011c00ULL, 0x104442040404200ULL, 0x511080202091021ULL, 0x4022401120400ULL,
        0x80c0040400080120ULL, 0x8040010040820802ULL, 0x480810700020090ULL, 0x102008e00040242ULL, 0x809005202050100ULL,
        0x8002024220104080ULL, 0x431008804142000ULL, 0x19001802081400ULL, 0x200014208040080ULL, 0x3308082008200100ULL,
        0x41010500040c020ULL, 0x4012020c04210308ULL, 0x208220a202004080ULL, 0x111040120082000ULL, 0x6803040141280a00ULL,
        0x2101004202410000ULL, 0x8200000041108022ULL, 0x21082088000ULL, 0x2410204010040ULL, 0x40100400809000ULL,
        0x822088220820214ULL, 0x40808090012004ULL, 0x910224040218c9ULL, 0x402814422015008ULL, 0x90014004842410ULL,
        0x1000042304105ULL, 0x10008830412a00ULL, 0x2520081090008908ULL, 0x40102000a0a60140ULL
    };

    constexpr int s_RookIndexBits[SQUARE_MAX] = {
        12, 11, 11, 11, 11, 11, 11, 12,
        11, 10, 10, 10, 10, 10, 10, 11,
        11, 10, 10, 10, 10, 10, 10, 11,
        11, 10, 10, 10, 10, 10, 10, 11,
        11, 10, 10, 10, 10, 10, 10, 11,
        11, 10, 10, 10, 10, 10, 10, 11,
        11, 10, 10, 10, 10, 10, 10, 11,
        12, 11, 11, 11, 11, 11, 11, 12,
    };

    constexpr int s_BishopIndexBits[SQUARE_MAX] = {
        6, 5, 5, 5, 5, 5, 5, 6,
        5, 5, 5, 5, 5, 5, 5, 5,
        5, 5, 7, 7, 7, 7, 5, 5,
        5, 5, 7, 9, 9, 7, 5, 5,
        5, 5, 7, 9, 9, 7, 5, 5,
        5, 5, 7, 7, 7, 7, 5, 5,
        5, 5, 5, 5, 5, 5, 5, 5,
        6, 5, 5, 5, 5, 5, 5, 6,
    };

    extern BitBoard s_NonSlidingAttacks[COLOR_MAX][PIECE_MAX][SQUARE_MAX];
    extern BitBoard s_SlidingAttacks[COLOR_MAX][PIECE_MAX][SQUARE_MAX];
    extern BitBoard s_Lines[SQUARE_MAX][SQUARE_MAX];

    extern BitBoard s_BishopMasks[SQUARE_MAX];
    extern BitBoard s_RookMasks[SQUARE_MAX];

    extern BitBoard s_RookTable[SQUARE_MAX][4096];
    extern BitBoard s_BishopTable[SQUARE_MAX][1024];

	void InitRays();
	void InitAttacks();

	BitBoard GetRay(Direction direction, SquareIndex fromSquare);

    template<Piece PIECE>
    inline BitBoard GetAttacks(SquareIndex fromSquare, BitBoard blockers)
    {
        static_assert(PIECE == PIECE_BISHOP || PIECE == PIECE_ROOK || PIECE == PIECE_QUEEN);
        if constexpr (PIECE == PIECE_BISHOP)
        {
            blockers &= s_BishopMasks[fromSquare];
            return s_BishopTable[fromSquare][(blockers * s_BishopMagics[fromSquare]) >> (64 - s_BishopIndexBits[fromSquare])];
        }
        if constexpr (PIECE == PIECE_ROOK)
        {
            blockers &= s_RookMasks[fromSquare];
            return s_RookTable[fromSquare][(blockers * s_RookMagics[fromSquare]) >> (64 - s_RookIndexBits[fromSquare])];
        }
        if constexpr (PIECE == PIECE_QUEEN)
        {
            return GetAttacks<PIECE_BISHOP>(fromSquare, blockers) | GetAttacks<PIECE_ROOK>(fromSquare, blockers);
        }
    }

    template<Piece PIECE>
    inline constexpr BitBoard GetAttacks(SquareIndex fromSquare, Color color)
    {
        static_assert(PIECE == PIECE_PAWN);
        return s_NonSlidingAttacks[color][PIECE][fromSquare];
    }

    template<Piece PIECE>
    inline constexpr BitBoard GetAttacks(SquareIndex fromSquare)
    {
        static_assert(PIECE == PIECE_KING || PIECE == PIECE_KNIGHT);
        return s_NonSlidingAttacks[COLOR_WHITE][PIECE][fromSquare];
    }

    inline BitBoard GetAttacksDynamic(Piece piece, SquareIndex fromSquare, BitBoard blockers)
    {
        switch (piece)
        {
        case PIECE_KNIGHT:
            return GetAttacks<PIECE_KNIGHT>(fromSquare);
        case PIECE_KING:
            return GetAttacks<PIECE_KING>(fromSquare);
        case PIECE_BISHOP:
            return GetAttacks<PIECE_BISHOP>(fromSquare, blockers);
        case PIECE_ROOK:
            return GetAttacks<PIECE_ROOK>(fromSquare, blockers);
        case PIECE_QUEEN:
            return GetAttacks<PIECE_QUEEN>(fromSquare, blockers);
        default:
            STORM_ASSERT(false, "Invalid piece type");
            break;
        }
        return ZERO_BB;
    }

    template<Color C>
    inline constexpr BitBoard GetPawnAttacks(BitBoard pawns)
    {
        return C == COLOR_WHITE ? Shift<NORTH_EAST>(pawns) | Shift<NORTH_WEST>(pawns)
                                : Shift<SOUTH_EAST>(pawns) | Shift<SOUTH_WEST>(pawns);
    }

    // Return a bitboard representing the squares between a and b not including a or b
    // If not on same rank/file/diagonal return 0
    inline BitBoard GetBitBoardBetween(SquareIndex a, SquareIndex b)
    {
        BitBoard result = s_Lines[a][b] & ((ALL_SQUARES_BB << a) ^ (ALL_SQUARES_BB << b));
        return BitBoard(result & (result - 1)); // Remove LSB
    }

    inline BitBoard GetLineBetween(SquareIndex a, SquareIndex b)
    {
        return s_Lines[a][b];
    }

    inline bool IsAligned(SquareIndex a, SquareIndex b, SquareIndex c)
    {
        return GetLineBetween(a, b) & c;
    }

}
