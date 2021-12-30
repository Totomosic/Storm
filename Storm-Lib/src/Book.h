#pragma once
#include "ZobristHash.h"
#include "Position.h"
#include "Move.h"

#include <vector>
#include <optional>
#include <unordered_map>

#ifdef SWIG
#define STORM_API
#define ValueType int16_t
#endif

namespace Storm
{

    struct STORM_API BookEntry
    {
    public:
        ZobristHash Hash = 0ULL;
        SquareIndex From = a1;
        SquareIndex To = a1;
        int Count = 1;
    };

    inline Move CreateMoveFromEntry(const BookEntry& entry, const Position& position)
    {
        if (FileOf(entry.From) == FILE_E && (FileOf(entry.To) == FILE_C || FileOf(entry.To) == FILE_G) &&
            TypeOf(position.GetPieceOnSquare(entry.From)) == PIECE_KING)
            return CreateMove(entry.From, entry.To, CASTLE);
        if (TypeOf(position.GetPieceOnSquare(entry.From)) == PIECE_PAWN &&
            RankOf(entry.To) == GetPromotionRank(position.ColorToMove))
            return CreateMove(entry.From, entry.To, PIECE_QUEEN);
        return CreateMove(entry.From, entry.To);
    }

    struct STORM_API BookEntryCollection
    {
    public:
        std::vector<BookEntry> Entries = {};
        int TotalCount = 0;

    public:
        // Weighted based on entry Count / TotalCount
        BookEntry PickRandom() const;
    };

    class STORM_API OpeningBook
    {
    private:
        size_t m_EntryCount;
        std::unordered_map<ZobristHash, BookEntryCollection> m_Entries;
        // Max depth of any entry
        int m_Cardinality;

    public:
        OpeningBook();

        size_t GetEntryCount() const;
        int GetCardinality() const;
        void SetCardinality(int cardinality);

        size_t CalculateFileSize() const;
        bool Serialize(void* buffer, size_t size) const;
        void WriteToFile(const std::string& filename) const;

        bool AppendFromFile(const std::string& filename);
        void AppendEntry(const BookEntry& entry);
        void Clear();

        std::optional<BookEntryCollection> Probe(const ZobristHash& hash) const;

    private:
        static constexpr size_t GetSerializedEntrySize()
        {
            return (sizeof(BookEntry::Hash.Hash) + sizeof(BookEntry::From) + sizeof(BookEntry::To) +
                    sizeof(BookEntry::Count));
        }
    };

}
