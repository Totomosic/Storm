#define CATCH_CONFIG_MAIN
#include "catch_amalgamated.hpp"
#include "Storm.h"

namespace Test
{

	using namespace Storm;

	TEST_CASE("ZobristHash", "[Hash]")
	{
		Init();
		ZobristHash hash;
		hash.SetFromPosition(CreateStartingPosition());

		uint64_t startingHash = hash.Hash;
		uint64_t currentHash = startingHash;

		hash.AddCastleKingside(COLOR_WHITE);
		REQUIRE(hash.Hash != currentHash);
		hash.RemoveCastleKingside(COLOR_WHITE);
		REQUIRE(hash.Hash == currentHash);

		hash.AddCastleKingside(COLOR_BLACK);
		REQUIRE(hash.Hash != currentHash);
		hash.RemoveCastleKingside(COLOR_BLACK);
		REQUIRE(hash.Hash == currentHash);

		hash.AddCastleQueenside(COLOR_WHITE);
		REQUIRE(hash.Hash != currentHash);
		hash.RemoveCastleQueenside(COLOR_WHITE);
		REQUIRE(hash.Hash == currentHash);

		hash.AddCastleQueenside(COLOR_BLACK);
		REQUIRE(hash.Hash != currentHash);
		hash.RemoveCastleQueenside(COLOR_BLACK);
		REQUIRE(hash.Hash == currentHash);

		hash.AddEnPassant(FILE_A);
		REQUIRE(hash.Hash != currentHash);
		hash.RemoveEnPassant(FILE_A);
		REQUIRE(hash.Hash == currentHash);

		hash.AddEnPassant(FILE_B);
		REQUIRE(hash.Hash != currentHash);
		hash.RemoveEnPassant(FILE_B);
		REQUIRE(hash.Hash == currentHash);

		hash.AddEnPassant(FILE_C);
		REQUIRE(hash.Hash != currentHash);
		hash.RemoveEnPassant(FILE_C);
		REQUIRE(hash.Hash == currentHash);

		hash.AddEnPassant(FILE_D);
		REQUIRE(hash.Hash != currentHash);
		hash.RemoveEnPassant(FILE_D);
		REQUIRE(hash.Hash == currentHash);

		hash.AddEnPassant(FILE_E);
		REQUIRE(hash.Hash != currentHash);
		hash.RemoveEnPassant(FILE_E);
		REQUIRE(hash.Hash == currentHash);

		hash.AddEnPassant(FILE_F);
		REQUIRE(hash.Hash != currentHash);
		hash.RemoveEnPassant(FILE_F);
		REQUIRE(hash.Hash == currentHash);

		hash.AddEnPassant(FILE_G);
		REQUIRE(hash.Hash != currentHash);
		hash.RemoveEnPassant(FILE_G);
		REQUIRE(hash.Hash == currentHash);

		hash.AddEnPassant(FILE_H);
		REQUIRE(hash.Hash != currentHash);
		hash.RemoveEnPassant(FILE_H);
		REQUIRE(hash.Hash == currentHash);
	}

	TEST_CASE("Perft", "[Perft]")
	{
		Init();
		Position position = CreateStartingPosition();
		Search search;

		size_t checks = 0;

		REQUIRE(search.Perft(position, 1) == 20);
		REQUIRE(search.Perft(position, 2) == 400);
		REQUIRE(search.Perft(position, 3) == 8902);
		REQUIRE(search.Perft(position, 4) == 197281);
		REQUIRE(search.Perft(position, 5) == 4865609);
		REQUIRE(search.Perft(position, 6) == 119060324);

		position = CreatePositionFromFEN("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");

		REQUIRE(search.Perft(position, 1) == 48);
		REQUIRE(search.Perft(position, 2) == 2039);
		REQUIRE(search.Perft(position, 3) == 97862);
		REQUIRE(search.Perft(position, 4) == 4085603);
		REQUIRE(search.Perft(position, 5) == 193690690);

		position = CreatePositionFromFEN("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -");

		REQUIRE(search.Perft(position, 1) == 14);
		REQUIRE(search.Perft(position, 2) == 191);
		REQUIRE(search.Perft(position, 3) == 2812);
		REQUIRE(search.Perft(position, 4) == 43238);
		REQUIRE(search.Perft(position, 5) == 674624);
		REQUIRE(search.Perft(position, 6) == 11030083);

		position = CreatePositionFromFEN("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");

		REQUIRE(search.Perft(position, 1) == 6);
		REQUIRE(search.Perft(position, 2) == 264);
		REQUIRE(search.Perft(position, 3) == 9467);
		REQUIRE(search.Perft(position, 4) == 422333);
		REQUIRE(search.Perft(position, 5) == 15833292);

		position = CreatePositionFromFEN("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");

		REQUIRE(search.Perft(position, 1) == 44);
		REQUIRE(search.Perft(position, 2) == 1486);
		REQUIRE(search.Perft(position, 3) == 62379);
		REQUIRE(search.Perft(position, 4) == 2103487);
		REQUIRE(search.Perft(position, 5) == 89941194);

		position = CreatePositionFromFEN("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");

		REQUIRE(search.Perft(position, 1) == 46);
		REQUIRE(search.Perft(position, 2) == 2079);
		REQUIRE(search.Perft(position, 3) == 89890);
		REQUIRE(search.Perft(position, 4) == 3894594);
		REQUIRE(search.Perft(position, 5) == 164075551);

		// From Stockfish
		position = CreatePositionFromFEN("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
		REQUIRE(search.Perft(position, 5) == 193690690);

		position = CreatePositionFromFEN("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -");
		REQUIRE(search.Perft(position, 6) == 11030083);

		position = CreatePositionFromFEN("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
		REQUIRE(search.Perft(position, 5) == 15833292);

		position = CreatePositionFromFEN("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
		REQUIRE(search.Perft(position, 5) == 89941194);

		position = CreatePositionFromFEN("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");
		REQUIRE(search.Perft(position, 5) == 164075551);

		position = CreatePositionFromFEN("r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ -");
		REQUIRE(search.Perft(position, 6) == 706045033);
	}

	TEST_CASE("Transposition", "[Transposition]")
	{
		TranspositionTable tt;

		Position position = CreateStartingPosition();

		TranspositionTableEntry tte;
		tte.Update(position.Hash, MOVE_NONE, 10, BOUND_UPPER, 160, -50);

		REQUIRE(tte.GetHash() == position.Hash);
		REQUIRE(tte.GetMove() == MOVE_NONE);
		REQUIRE(tte.GetDepth() == 10);
		REQUIRE(tte.GetValue() == 160);
		REQUIRE(tte.GetBound() == BOUND_UPPER);
		REQUIRE(tte.GetStaticEvaluation() == -50);

		bool found;
		TranspositionTableEntry* entry = tt.GetEntry(position.Hash, found);
		REQUIRE(!found);
		*entry = tte;
		TranspositionTableEntry* newEntry = tt.GetEntry(position.Hash, found);
		REQUIRE(newEntry == entry);
		REQUIRE(found);
		REQUIRE(newEntry->GetHash() == position.Hash);
		REQUIRE(newEntry->GetMove() == MOVE_NONE);
		REQUIRE(newEntry->GetDepth() == 10);
		REQUIRE(newEntry->GetValue() == 160);
		REQUIRE(newEntry->GetBound() == BOUND_UPPER);

		tte.Update(position.Hash, MOVE_NONE, 0, BOUND_EXACT, -23, 10);
		REQUIRE(tte.GetHash() == position.Hash);
		REQUIRE(tte.GetMove() == MOVE_NONE);
		REQUIRE(tte.GetDepth() == 0);
		REQUIRE(tte.GetValue() == -23);
		REQUIRE(tte.GetBound() == BOUND_EXACT);
		REQUIRE(tte.GetStaticEvaluation() == 10);

		Move move = CreateMove(e2, e4);

		tte.Update(position.Hash, move, 5, BOUND_EXACT, VALUE_MATE - 10, -2000);
		REQUIRE(tte.GetHash() == position.Hash);
		REQUIRE(tte.GetMove() == move);
		REQUIRE(tte.GetDepth() == 5);
		REQUIRE(tte.GetValue() == VALUE_MATE - 10);
		REQUIRE(tte.GetBound() == BOUND_EXACT);
		REQUIRE(tte.GetStaticEvaluation() == -2000);
	}

	TEST_CASE("Checks", "[Check]")
	{
		Init();
		Position position = CreatePositionFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

		REQUIRE(position.InCheck() == false);

		position = CreatePositionFromFEN("rnbqkbnr/ppppQppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

		REQUIRE(position.InCheck() == false);

		position = CreatePositionFromFEN("rnbqkbnr/ppppQppp/8/8/8/8/PPPPrPPP/RNBQKBNR w KQkq - 0 1");

		REQUIRE(position.InCheck() == true);

		position = CreatePositionFromFEN("rnbqkbnr/pppppppp/8/1B6/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 1");

		REQUIRE(position.InCheck() == false);

		UndoInfo undo;
		StateInfo st;
		Move move = UCI::CreateMoveFromString(position, "b5d7");
		position.ApplyMove(move, st, &undo);

		REQUIRE(position.InCheck() == true);
	}

	TEST_CASE("MoveGeneration", "[MOVEGEN]")
	{
		Init();

		for (const std::string& fen : {
				"rnbqkbnr/ppp2ppp/3p4/1B2p3/3PP3/8/PPP2PPP/RNBQK1NR b KQkq - 1 3",
				"1k1r4/1bp1qp2/Rp2p2p/4P3/1P1P4/Q2B2r1/1P3P1P/R5K1 w - - 0 27",
				"Rk1r4/1b2qp2/1pp1p2p/4P3/1P1P4/Q2B2P1/1P3P2/R5K1 b - - 1 28",
				"8/R1k2p2/1pp1p2p/1P2P3/3P4/6P1/1P3P2/6K1 b - - 0 34",
				"1k2Q3/8/1p1R4/7p/1p1P4/6P1/1P3P2/6K1 b - - 0 41",
				"8/1k1Q4/1p1R4/7p/1p1P4/6P1/1P3P2/6K1 b - - 2 42",
				"k2Q4/8/1p1R4/7p/1p1P4/6P1/1P3P2/6K1 b - - 4 43",
				"8/1k2Q3/1p1R4/7p/1p1P4/6P1/1P3P2/6K1 b - - 6 44",
				"8/8/kp1R4/7p/1p1P4/6P1/1P2QP2/6K1 b - - 8 45",
				"8/8/1p6/k2R3p/1p1P4/6P1/1P2QP2/6K1 b - - 10 46",
				"8/8/8/kQ1R3p/1p1P4/6P1/1P3P2/6K1 b - - 0 47",
			})
		{
			Position position = CreatePositionFromFEN(fen);
			Move moves[MAX_MOVES];
			MoveList list(moves);
			list.FillLegal<ALL>(position);

			Move qMoves[MAX_MOVES];
			MoveList qList(qMoves);
			
			if (position.ColorToMove == COLOR_WHITE)
				qList.Fill(GenerateAll<COLOR_WHITE, EVASIONS>(position, qList.GetStart()));
			else
				qList.Fill(GenerateAll<COLOR_BLACK, EVASIONS>(position, qList.GetStart()));

			for (Move mv : list)
			{
				REQUIRE(std::find(qList.begin(), qList.end(), mv) != qList.end());
			}
		}
	}

	TEST_CASE("SEE", "[SEE]")
	{
		Init();

		UndoInfo undo;
		StateInfo info;
		Position position = CreateStartingPosition();
		position.Reset(&info);
		position.ApplyMove(CreateMove(e2, e4), info, &undo);
		position.ApplyMove(CreateMove(a7, a6), info, &undo);

		REQUIRE(!position.SeeGE(CreateMove(f1, a6)));

	}

	/*TEST_CASE("Mirror", "[Evaluation]")
	{
		Init();

		for (const std::string& fen : {
				"r3qb2/2n2R1p/1p6/p2kBQ2/8/8/PPP2PRP/6K1 w - - 7 51",
				"r4b2/2R4p/1p6/p2kqQ2/8/8/PPP2PRP/6K1 w - - 0 52",
				"r1b2rk1/ppp2qpp/4p3/2P1Pn2/2Q5/3BP2R/PP4P1/R3K1N1 b Q - 6 21",
				"r4rk1/pppb1qpp/4p3/2P1Pn2/2Q1B3/4P2R/PP4P1/R3K1N1 b Q - 8 22",
				"rnb1kbnr/p1ppqp1p/1p2p1p1/4P3/3P4/3B1N2/PPP2PPP/RNBQK2R w KQkq - 1 6",
				"rn2kbnr/pbppqp1p/1p2p1p1/4P3/3P4/3B1N2/PPP2PPP/RNBQ1K1R w kq - 3 7",
				"rnb1kbnr/p1ppqp1p/1p2p1p1/4P3/3P4/3B1N2/PPP2PPP/RNBQ1K1R b kq - 2 6",
				"1k1r4/pp1b1R2/3q2pp/4p3/2B5/4Q3/PPP2B2/2K5 b - -",
				"3r1k2/4npp1/1ppr3p/p6P/P2PPPP1/1NR5/5K2/2R5 w - -",
				"2q1rr1k/3bbnnp/p2p1pp1/2pPp3/PpP1P1P1/1P2BNNP/2BQ1PRK/7R b - -",
				"rnbqkb1r/p3pppp/1p6/2ppP3/3N4/2P5/PPP1QPPP/R1B1KB1R w KQkq -",
				"r1b2rk1/2q1b1pp/p2ppn2/1p6/3QP3/1BN1B3/PPP3PP/R4RK1 w - -",
				"2r3k1/pppR1pp1/4p3/4P1P1/5P2/1P4K1/P1P5/8 w - -",
				"1nk1r1r1/pp2n1pp/4p3/q2pPp1N/b1pP1P2/B1P2R2/2P1B1PP/R2Q2K1 w - -",
				"4b3/p3kp2/6p1/3pP2p/2pP1P2/4K1P1/P3N2P/8 w - -",
				"2kr1bnr/pbpq4/2n1pp2/3p3p/3P1P1B/2N2N1Q/PPP3PP/2KR1B1R w - -",
				"3rr1k1/pp3pp1/1qn2np1/8/3p4/PP1R1P2/2P1NQPP/R1B3K1 b - -",
				"2r1nrk1/p2q1ppp/bp1p4/n1pPp3/P1P1P3/2PBB1N1/4QPPP/R4RK1 w - -",
				"r3r1k1/ppqb1ppp/8/4p1NQ/8/2P5/PP3PPP/R3R1K1 b - -",
				"r2q1rk1/4bppp/p2p4/2pP4/3pP3/3Q4/PP1B1PPP/R3R1K1 w - -",
				"rnb2r1k/pp2p2p/2pp2p1/q2P1p2/8/1Pb2NP1/PB2PPBP/R2Q1RK1 w - -",
				"2r3k1/1p2q1pp/2b1pr2/p1pp4/6Q1/1P1PP1R1/P1PN2PP/5RK1 w - -",
				"r1bqkb1r/4npp1/p1p4p/1p1pP1B1/8/1B6/PPPN1PPP/R2Q1RK1 w kq -",
				"r2q1rk1/1ppnbppp/p2p1nb1/3Pp3/2P1P1P1/2N2N1P/PPB1QP2/R1B2RK1 b - -",
				"r1bq1rk1/pp2ppbp/2np2p1/2n5/P3PP2/N1P2N2/1PB3PP/R1B1QRK1 b - -",
				"3rr3/2pq2pk/p2p1pnp/8/2QBPP2/1P6/P5PP/4RRK1 b - -",
				"r4k2/pb2bp1r/1p1qp2p/3pNp2/3P1P2/2N3P1/PPP1Q2P/2KRR3 w - -",
				"3rn2k/ppb2rpp/2ppqp2/5N2/2P1P3/1P5Q/PB3PPP/3RR1K1 w - -",
				"2r2rk1/1bqnbpp1/1p1ppn1p/pP6/N1P1P3/P2B1N1P/1B2QPP1/R2R2K1 b - -",
				"r1bqk2r/pp2bppp/2p5/3pP3/P2Q1P2/2N1B3/1PP3PP/R4RK1 b kq -",
				"r2qnrnk/p2b2b1/1p1p2pp/2pPpp2/1PP1P3/PRNBB3/3QNPPP/5RK1 w - -",
				"2r1r1k1/ppp3p1/2N1p2p/2nqQ3/8/P7/1PPB1PPP/3RR1K1 b - - 0 23",
				"2r3k1/1pp1r1p1/p1q1R2p/1n6/3Q4/PPBR4/2P2PPP/6K1 w - - 1 31",
				"r5k1/2p1r1p1/p1p4p/8/R7/PP4BP/2P2PP1/6K1 b - - 6 36",
				"8/3kb3/1p1r4/p1p1Q3/2P4p/4P3/P4PP1/5K2 b - - 8 43",
				"4n3/1p1q1kp1/1Q3p2/8/p4B1r/P7/1PP2P2/4R1K1 w - - 0 35",
				"r1bqk2r/pppp1ppp/2n1p3/bB1nP3/3P4/2P2N2/PP3PPP/RNBQK2R w KQkq - 1 7",
				"2k5/ppp3pp/4Kp2/2p5/2rn1r2/8/P4PPP/2R5 w - - 2 33",
				"8/8/8/4Q2p/3PK3/2k3P1/8/8 w - - 5 68",
				"4r1k1/3nbppp/bqr2B2/p7/2p5/6P1/P2N1PBP/1R1QR1K1 b - - 0 1",
				"2r2knr/2P5/1R3pp1/p4p1p/3P1B2/5N2/PP1N1PPP/3QR1K1 w - - 0 20",
				"r3r3/1ppq1ppk/p6p/3pP2Q/2bPnB1N/6R1/P1P2PPP/6RK w - - 1 30",
				"8/3b1kb1/2p2p1p/p2p4/1p3RpN/4r3/PPP3PP/3R2K1 b - - 1 32",
			})
		{
			Position position = CreatePositionFromFEN(fen);
 
			ValueType eval = Evaluate(position, position.TeamToPlay);
			REQUIRE(eval == Evaluate(MirrorPosition(position), OtherTeam(position.TeamToPlay)));
		}
	}*/

	TEST_CASE("PGN", "[FORMATTING]")
	{
		Init();
		Position position = CreatePositionFromFEN("r2q3k/p2P3p/1p3p2/3QP1r1/8/B7/P5PP/2R3K1 w - -");
		Move move = PGN::CreateMoveFromString(position, "Qxa8");
		REQUIRE(move != MOVE_NONE);
		REQUIRE(GetFromSquare(move) == d5);
		REQUIRE(GetToSquare(move) == a8);

		move = CreateMove(c1, c8);
		std::string format = PGN::FormatMove(move, position);
		REQUIRE(format == "Rc8");
	}

}
