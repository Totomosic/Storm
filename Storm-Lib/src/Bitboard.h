#pragma once
#include "Types.h"
#include <sstream>
#include <iostream>

#ifdef SWIG
#define STORM_API
#define ValueType int16_t
#endif

namespace Storm
{

	enum BitBoard : uint64_t
	{
		ZERO_BB = 0ULL,
		RANK_1_BB = 0xffULL,
		RANK_2_BB = 0xff00ULL,
		RANK_3_BB = 0xff0000ULL,
		RANK_4_BB = 0xff000000ULL,
		RANK_5_BB = 0xff00000000ULL,
		RANK_6_BB = 0xff0000000000ULL,
		RANK_7_BB = 0xff000000000000ULL,
		RANK_8_BB = 0xff00000000000000ULL,

		FILE_A_BB = 0x101010101010101ULL,
		FILE_B_BB = 0x202020202020202ULL,
		FILE_C_BB = 0x404040404040404ULL,
		FILE_D_BB = 0x808080808080808ULL,
		FILE_E_BB = 0x1010101010101010ULL,
		FILE_F_BB = 0x2020202020202020ULL,
		FILE_G_BB = 0x4040404040404040ULL,
		FILE_H_BB = 0x8080808080808080ULL,

		ALL_SQUARES_BB = 0xFFFFFFFFFFFFFFFFULL,
	};

	inline constexpr BitBoard operator&(BitBoard left, BitBoard right) { return BitBoard(uint64_t(left) & uint64_t(right)); }
	inline constexpr BitBoard operator|(BitBoard left, BitBoard right) { return BitBoard(uint64_t(left) | uint64_t(right)); }
	inline constexpr BitBoard operator^(BitBoard left, BitBoard right) { return BitBoard(uint64_t(left) ^ uint64_t(right)); }
	inline constexpr BitBoard operator>>(BitBoard left, int right) { return BitBoard(uint64_t(left) >> right); }
	inline constexpr BitBoard operator<<(BitBoard left, int right) { return BitBoard(uint64_t(left) << right); }
	inline constexpr BitBoard operator~(BitBoard left) { return BitBoard(~uint64_t(left)); }

	inline constexpr BitBoard& operator&=(BitBoard& left, BitBoard right) { left = left & right; return left; }
	inline constexpr BitBoard& operator|=(BitBoard& left, BitBoard right) { left = left | right; return left; }
	inline constexpr BitBoard& operator^=(BitBoard& left, BitBoard right) { left = left ^ right; return left; }
	inline constexpr BitBoard& operator>>=(BitBoard& left, int right) { left = left >> right; return left; }
	inline constexpr BitBoard& operator<<=(BitBoard& left, int right) { left = left << right; return left; }

	constexpr BitBoard SQUARE_BITBOARDS[SQUARE_MAX] = {
		BitBoard(1ULL << a1), BitBoard(1ULL << b1), BitBoard(1ULL << c1), BitBoard(1ULL << d1), BitBoard(1ULL << e1), BitBoard(1ULL << f1), BitBoard(1ULL << g1), BitBoard(1ULL << h1),
		BitBoard(1ULL << a2), BitBoard(1ULL << b2), BitBoard(1ULL << c2), BitBoard(1ULL << d2), BitBoard(1ULL << e2), BitBoard(1ULL << f2), BitBoard(1ULL << g2), BitBoard(1ULL << h2),
		BitBoard(1ULL << a3), BitBoard(1ULL << b3), BitBoard(1ULL << c3), BitBoard(1ULL << d3), BitBoard(1ULL << e3), BitBoard(1ULL << f3), BitBoard(1ULL << g3), BitBoard(1ULL << h3),
		BitBoard(1ULL << a4), BitBoard(1ULL << b4), BitBoard(1ULL << c4), BitBoard(1ULL << d4), BitBoard(1ULL << e4), BitBoard(1ULL << f4), BitBoard(1ULL << g4), BitBoard(1ULL << h4),
		BitBoard(1ULL << a5), BitBoard(1ULL << b5), BitBoard(1ULL << c5), BitBoard(1ULL << d5), BitBoard(1ULL << e5), BitBoard(1ULL << f5), BitBoard(1ULL << g5), BitBoard(1ULL << h5),
		BitBoard(1ULL << a6), BitBoard(1ULL << b6), BitBoard(1ULL << c6), BitBoard(1ULL << d6), BitBoard(1ULL << e6), BitBoard(1ULL << f6), BitBoard(1ULL << g6), BitBoard(1ULL << h6),
		BitBoard(1ULL << a7), BitBoard(1ULL << b7), BitBoard(1ULL << c7), BitBoard(1ULL << d7), BitBoard(1ULL << e7), BitBoard(1ULL << f7), BitBoard(1ULL << g7), BitBoard(1ULL << h7),
		BitBoard(1ULL << a8), BitBoard(1ULL << b8), BitBoard(1ULL << c8), BitBoard(1ULL << d8), BitBoard(1ULL << e8), BitBoard(1ULL << f8), BitBoard(1ULL << g8), BitBoard(1ULL << h8),
	};

	constexpr BitBoard GetSquareBB(SquareIndex square)
	{
		return SQUARE_BITBOARDS[square];
	}

	inline constexpr BitBoard operator&(BitBoard left, SquareIndex right) { return left & SQUARE_BITBOARDS[right]; }
	inline constexpr BitBoard operator|(BitBoard left, SquareIndex right) { return left | SQUARE_BITBOARDS[right]; }
	inline constexpr BitBoard operator^(BitBoard left, SquareIndex right) { return left ^ SQUARE_BITBOARDS[right]; }
	inline constexpr BitBoard& operator&=(BitBoard& left, SquareIndex right) { return left &= SQUARE_BITBOARDS[right]; }
	inline constexpr BitBoard& operator|=(BitBoard& left, SquareIndex right) { return left |= SQUARE_BITBOARDS[right]; }
	inline constexpr BitBoard& operator^=(BitBoard& left, SquareIndex right) { return left ^= SQUARE_BITBOARDS[right]; }
	inline constexpr BitBoard operator&(SquareIndex left, BitBoard right) { return right & SQUARE_BITBOARDS[left]; }
	inline constexpr BitBoard operator|(SquareIndex left, BitBoard right) { return right | SQUARE_BITBOARDS[left]; }
	inline constexpr BitBoard operator^(SquareIndex left, BitBoard right) { return right ^ SQUARE_BITBOARDS[left]; }
	inline constexpr BitBoard operator~(SquareIndex left) { return ~SQUARE_BITBOARDS[left]; }

	inline constexpr BitBoard operator|(SquareIndex left, SquareIndex right) { return SQUARE_BITBOARDS[left] | SQUARE_BITBOARDS[right]; }

	constexpr BitBoard FILE_MASKS[FILE_MAX] = {
		FILE_A_BB,
		FILE_B_BB,
		FILE_C_BB,
		FILE_D_BB,
		FILE_E_BB,
		FILE_F_BB,
		FILE_G_BB,
		FILE_H_BB,
	};

	constexpr BitBoard RANK_MASKS[RANK_MAX] = {
		RANK_1_BB,
		RANK_2_BB,
		RANK_3_BB,
		RANK_4_BB,
		RANK_5_BB,
		RANK_6_BB,
		RANK_7_BB,
		RANK_8_BB,
	};

	template<Direction D>
	inline constexpr BitBoard Shift(BitBoard b)
	{
		return  D == NORTH ?		(b)              << 8 : D == SOUTH ?		(b)				 >> 8
			  : D == EAST ?			(b & ~FILE_H_BB) << 1 : D == WEST ?			(b & ~FILE_A_BB) >> 1
			  : D == NORTH_EAST ?	(b & ~FILE_H_BB) << 9 : D == NORTH_WEST ?	(b & ~FILE_A_BB) << 7
			  : D == SOUTH_EAST ?	(b & ~FILE_H_BB) >> 7 : D == SOUTH_WEST ?	(b & ~FILE_A_BB) >> 9
			  : ZERO_BB;
	}

	inline constexpr SquareIndex GetEnpassantSquare(SquareIndex toSquare, Color color)
	{
		return color == COLOR_WHITE ? SquareIndex(toSquare - 8) : SquareIndex(toSquare + 8);
	}

#ifdef STORM_PLATFORM_WINDOWS
	// a1 -> h8
	inline SquareIndex LeastSignificantBit(BitBoard board)
	{
		STORM_ASSERT(board != ZERO_BB, "Invalid BitBoard");
		unsigned long lsb;
		_BitScanForward64(&lsb, board);
		return SquareIndex(lsb);
	}

	// h8 -> a1
	inline SquareIndex MostSignificantBit(BitBoard board)
	{
		STORM_ASSERT(board != ZERO_BB, "Invalid BitBoard");
		unsigned long msb;
		_BitScanReverse64(&msb, board);
		return SquareIndex(msb);
	}

	inline BitBoard FlipVertically(BitBoard board)
	{
		return BitBoard(_byteswap_uint64(board));
	}

	inline int Popcount(BitBoard board)
	{
		return int(__popcnt64(board));
	}
#elif STORM_PLATFORM_LINUX
	inline SquareIndex LeastSignificantBit(BitBoard board)
	{
		STORM_ASSERT(board != ZERO_BB, "Invalid BitBoard");
		return SquareIndex(__builtin_ffsll(board) - 1);
	}

	inline SquareIndex MostSignificantBit(BitBoard board)
	{
		STORM_ASSERT(board != ZERO_BB, "Invalid BitBoard");
		return SquareIndex(63 - __builtin_clzll(board));
	}

#ifndef EMSCRIPTEN
	inline BitBoard FlipVertically(BitBoard board)
	{
		return BitBoard(__bswap_64(board));
	}
#endif

	inline int Popcount(BitBoard board)
	{
		return int(__builtin_popcountll(board));
	}
#endif

	inline SquareIndex PopLeastSignificantBit(BitBoard& board)
	{
		STORM_ASSERT(board != ZERO_BB, "Invalid BitBoard");
		const SquareIndex square = LeastSignificantBit(board);
		board &= BitBoard(board - 1);
		return square;
	}

	constexpr bool MoreThanOne(BitBoard board)
	{
		return bool(board & BitBoard(board - 1));
	}

	inline std::string FormatBitBoard(BitBoard board)
	{
		std::stringstream stream;
		stream << "   | A B C D E F G H" << std::endl;
		stream << "---------------------" << std::endl;
		for (Rank rank = RANK_8; rank >= RANK_1; rank--)
		{
			stream << ' ' << (char)('1' + (rank - RANK_1)) << ' ' << '|' << ' ';
			for (File file = FILE_A; file < FILE_MAX; file++)
			{
				stream << ((board & CreateSquare(file, rank)) ? '1' : '0') << ' ';
			}
			if (rank != RANK_1)
				stream << std::endl;
		}
		return stream.str();
	}

	inline std::ostream& operator<<(std::ostream& stream, BitBoard board)
	{
		stream << FormatBitBoard(board);
		return stream;
	}

}
