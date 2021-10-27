#pragma once
#include "Position.h"
#include "MoveGeneration.h"

#include <unordered_map>
#include <vector>

#ifdef SWIG
#define STORM_API
#define ValueType int16_t
#endif

namespace Storm
{

	std::vector<std::string> Split(const std::string& str, const std::string& delimiter);

	class STORM_API UCI
	{
	public:
		UCI() = delete;

		static char PieceToString(Piece type, Color color);
		static SquareIndex SquareFromString(const std::string& square);
		static std::string SquareToString(SquareIndex square);
		static std::string FormatMove(Move move);
		static Move CreateMoveFromString(const Position& position, const std::string& move);
	};

	struct STORM_API PGNMatch
	{
	public:
		std::unordered_map<std::string, std::string> Tags;
		Position InitialPosition;
		std::vector<Move> Moves;
	};

	class STORM_API PGN
	{
	public:
		PGN() = delete;

		// PGN formatting is context dependent so also needs the position to be specified
		// The position must be provided before the move is played
		static std::string FormatMove(Move move, const Position& position);
		static Move CreateMoveFromString(const Position& position, const std::string& pgnString);

		static std::vector<PGNMatch> ReadFromString(const std::string& pgn);
		static std::vector<PGNMatch> ReadFromFile(const std::string& filename);

	private:
		static char GetFileName(File file);
		static char GetRankName(Rank rank);
		static File GetFileFromName(char name);
		static Rank GetRankFromName(char name);
		static std::string GetPieceName(Piece piece);
		static Piece GetPieceFromName(char piece);
		static bool IsCapital(char c);
	};

}
