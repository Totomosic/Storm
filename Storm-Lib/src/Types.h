#pragma once
#pragma warning( disable : 26812 )
#include "Logging.h"

#define STORM_DEFINE_INC_OPERATORS(T)	\
inline constexpr T& operator++(T& value) { value = T(value + 1); return value; }	\
inline constexpr T operator++(T& value, int) { value = T(value + 1); return T(value - 1); }	\
inline constexpr T& operator--(T& value) { value = T(value - 1); return value; }	\
inline constexpr T operator--(T& value, int) { value = T(value - 1); return T(value + 1); }

#define STORM_DEFINE_OPERATORS(T, OTHER_T)	\
inline constexpr T operator+(T left, OTHER_T right) { return T(OTHER_T(left) + right); }	\
inline constexpr T operator-(T left, OTHER_T right) { return T(OTHER_T(left) - right); }

#ifdef SWIG
#define STORM_API
#define ValueType int16_t
#endif

namespace Storm
{

	STORM_API enum Color : int8_t
	{
		COLOR_WHITE,
		COLOR_BLACK,
		COLOR_MAX,
	};

	constexpr Color OtherColor(Color c)
	{
		return c == COLOR_WHITE ? COLOR_BLACK : COLOR_WHITE;
	}

	enum File : int8_t
	{
		FILE_A,
		FILE_B,
		FILE_C,
		FILE_D,
		FILE_E,
		FILE_F,
		FILE_G,
		FILE_H,
		FILE_MAX,
	};

	STORM_DEFINE_INC_OPERATORS(File);

	enum Rank : int8_t
	{
		RANK_1,
		RANK_2,
		RANK_3,
		RANK_4,
		RANK_5,
		RANK_6,
		RANK_7,
		RANK_8,
		RANK_MAX,
	};

	STORM_DEFINE_INC_OPERATORS(Rank);

	enum SquareIndex : int8_t
	{
		a1, b1, c1, d1, e1, f1, g1, h1,
		a2, b2, c2, d2, e2, f2, g2, h2,
		a3, b3, c3, d3, e3, f3, g3, h3,
		a4, b4, c4, d4, e4, f4, g4, h4,
		a5, b5, c5, d5, e5, f5, g5, h5,
		a6, b6, c6, d6, e6, f6, g6, h6,
		a7, b7, c7, d7, e7, f7, g7, h7,
		a8, b8, c8, d8, e8, f8, g8, h8,
		SQUARE_MAX,
		SQUARE_INVALID,
	};

	STORM_DEFINE_OPERATORS(SquareIndex, int);
	STORM_DEFINE_INC_OPERATORS(SquareIndex);

	constexpr File FileOf(SquareIndex square)
	{
		return File(square & 0x7);
	}

	constexpr Rank RankOf(SquareIndex square)
	{
		return Rank(square >> 0x3);
	}

	constexpr File OppositeFile(File file)
	{
		return File(~uint8_t(file) & 0x7);
	}

	constexpr Rank OppositeRank(Rank rank)
	{
		return Rank(~uint8_t(rank) & 0x7);
	}

	constexpr SquareIndex OppositeSquare(SquareIndex square)
	{
		return SquareIndex(square ^ 56);
	}

	constexpr SquareIndex CreateSquare(File file, Rank rank)
	{
		return SquareIndex(file + FILE_MAX * rank);
	}

	constexpr Rank GetPromotionRank(Color color)
	{
		return color == COLOR_WHITE ? RANK_8 : RANK_1;
	}

	constexpr SquareIndex SquareBehind(SquareIndex square, Color color)
	{
		return color == COLOR_WHITE ? SquareIndex(square - FILE_MAX) : SquareIndex(square + FILE_MAX);
	}

	constexpr SquareIndex SquareInFront(SquareIndex square, Color color)
	{
		return color == COLOR_WHITE ? SquareIndex(square + FILE_MAX) : SquareIndex(square - FILE_MAX);
	}

	enum Piece : int8_t
	{
		PIECE_NONE,
		PIECE_START = 1, PIECE_PAWN = 1,
		PIECE_KNIGHT,
		PIECE_BISHOP,
		PIECE_ROOK,
		PIECE_QUEEN,
		PIECE_KING,
		PIECE_MAX,
		PIECE_ALL,
	};
	
	STORM_DEFINE_INC_OPERATORS(Piece);

	enum ColorPiece : uint8_t
	{
		COLOR_PIECE_NONE = 0,
		WHITE_PAWN = PIECE_PAWN, WHITE_KNIGHT, WHITE_BISHOP, WHITE_ROOK, WHITE_QUEEN, WHITE_KING,
		BLACK_PAWN = PIECE_PAWN + 8, BLACK_KNIGHT, BLACK_BISHOP, BLACK_ROOK, BLACK_QUEEN, BLACK_KING,
		COLOR_PIECE_COUNT = 12,
	};

	constexpr ColorPiece CreatePiece(Piece type, Color color)
	{
		return ColorPiece(type + color * 8);
	}

	constexpr Piece TypeOf(ColorPiece piece)
	{
		return Piece(piece > 8 ? piece - 8 : piece);
	}

	constexpr Color ColorOf(ColorPiece piece)
	{
		return Color(piece > 8);
	}

	enum Direction : int8_t
	{
		NORTH,
		SOUTH,
		EAST,
		WEST,
		NORTH_EAST,
		NORTH_WEST,
		SOUTH_EAST,
		SOUTH_WEST,
		DIRECTION_MAX,
	};

	constexpr int PIECE_COUNT = PIECE_MAX - PIECE_START;

#ifndef SWIG
	using ValueType = int16_t;
#endif

}
