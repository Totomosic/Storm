#include "Search.h"
#include <chrono>

namespace Storm
{

    Search::Search()
    {
    }

    size_t Search::Perft(const Position& position, int depth)
    {
        size_t total = 0;
        Move moves[MAX_MOVES];
        Move* end;
        if (position.ColorToMove == COLOR_WHITE)
            end = GenerateAll<COLOR_WHITE, ALL>(position, moves);
        else
            end = GenerateAll<COLOR_BLACK, ALL>(position, moves);

        auto startTime = std::chrono::high_resolution_clock::now();
        Move* it = moves;

        while (it != end)
        {
            if (position.IsLegal(*it))
            {
                Position movedPosition = position;
                movedPosition.ApplyMove(*it, position.GivesCheck(*it));
                size_t perft = PerftPosition(movedPosition, depth - 1);
                total += perft;

                std::cout << UCI::FormatMove(*it) << ": " << perft << std::endl;
            }
            it++;
        }
        auto endTime = std::chrono::high_resolution_clock::now();
        auto elapsed = endTime - startTime;
        std::cout << "====================================" << std::endl;
        std::cout << "Total Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() << "ms" << std::endl;
        std::cout << "Total Nodes: " << total << std::endl;
        std::cout << "Nodes per Second: " << (size_t)(total / (double)std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count() * 1e9) << std::endl;
        return total;
    }

    size_t Search::PerftPosition(Position& position, int depth)
    {
        if (depth <= 0)
            return 1;
        Move moves[MAX_MOVES];
        if (depth == 1)
        {
            Move* end;
            if (position.ColorToMove == COLOR_WHITE)
                end = GenerateAll<COLOR_WHITE, ALL>(position, moves);
            else
                end = GenerateAll<COLOR_BLACK, ALL>(position, moves);
            Move* it = moves;
            size_t total = 0;
            while (it != end)
            {
                if (position.IsLegal(*it))
                {
                    total++;
                }
                it++;
            }
            return total;
        }

        Move* end;
        if (position.ColorToMove == COLOR_WHITE)
            end = GenerateAll<COLOR_WHITE, ALL>(position, moves);
        else
            end = GenerateAll<COLOR_BLACK, ALL>(position, moves);
        Move* it = moves;
        size_t nodes = 0;

        while (it != end)
        {
            if (position.IsLegal(*it))
            {
                Position movedPosition = position;
                movedPosition.ApplyMove(*it, position.GivesCheck(*it));
                nodes += PerftPosition(movedPosition, depth - 1);
            }
            it++;
        }
        return nodes;
    }

}
